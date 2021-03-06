import argparse
import pandas as pd
import pickle
import sys
import torch.nn.functional as F

from tqdm import tqdm

sys.path.insert(0, '/home/huanyuan/code/demo/Speech/KWS')
from impl.pred_pyimpl import kws_load_model, dataset_add_noise, model_predict
from dataset.kws.dataset_helper import *
from utils.train_tools import *


def longterm_audio_predict(cfg, net, audio_idx, audio_file, audio_mode, audio_label, audio_label_idx, timeshift_ms, average_window_duration_ms):
    # init 
    input_dir = os.path.join(cfg.general.data_dir, '../dataset_{}_{}'.format(cfg.general.version, cfg.general.date), 'dataset_audio', audio_mode)
    input_dir = os.path.join(input_dir, audio_label)
    num_classes = cfg.dataset.label.num_classes
    sample_rate = cfg.dataset.sample_rate
    clip_duration_ms = cfg.dataset.clip_duration_ms
    desired_samples = int(sample_rate * clip_duration_ms / 1000)

    # load data
    data, filename = load_preload_audio(audio_file, audio_idx, audio_label, audio_label_idx, input_dir)

    # # debug
    # librosa.output.write_wav(os.path.join("/home/huanyuan/model/model_10_30_25_21/model/kws_xiaoyu_res15_10272020/testing/", filename.split('.')[0] + '.wav'), data, sr=sample_rate)

    # alignment data
    data = np.pad(data, (0, max(0, desired_samples - len(data))), "constant")

    # add noise for _silence_ label 
    if audio_label == SILENCE_LABEL:
        data = dataset_add_noise(cfg, data, bool_silence_label=True)

    # calculate the average score across all the results
    data_list = []
    if len(data) > desired_samples:
        timeshift_samples = int(sample_rate * timeshift_ms / 1000)
        data_number = 1 + (len(data) - desired_samples) // timeshift_samples
        for data_idx in range(data_number):
            data_list.append(data[timeshift_samples * data_idx: timeshift_samples * data_idx + desired_samples])
    else:
        data_list.append(data)
    
    score_list = []
    for data_idx in range(len(data_list)):
        # # debug
        # librosa.output.write_wav(os.path.join("/home/huanyuan/model/model_10_30_25_21/model/kws_xiaoyu_res15_10272020/testing/", str(data_idx) + '.wav'),  
        #     data_list[data_idx], sr=sample_rate)
        score = model_predict(cfg, net, data_list[data_idx])
        score_list.append(score[0])

    average_window_length = 1 + average_window_duration_ms // timeshift_ms
    if len(score_list) > average_window_length:
        windows_number = 1 + len(score_list) - average_window_length
        # Calculate the average score across all the results in the window.
        average_scores_list = []
        for windows_idx in range(windows_number):
            score_list_window = score_list[windows_idx: windows_idx + average_window_length]
            average_scores = np.zeros(num_classes)
            for score in score_list_window:
                for idx in range(num_classes):
                    average_scores[idx] += score[idx] / len(score_list_window)
            average_scores_list.append(average_scores)
        # Sort the averaged results.
        average_scores_list = sorted(average_scores_list, key=lambda p: p[audio_label_idx])
        average_scores = average_scores_list[0]
    else:
        average_scores = np.zeros(num_classes)
        for score in score_list:
            for idx in range(num_classes):
                average_scores[idx] += score[idx] / len(score_list)

    pred = np.argmax(average_scores)
    return pred, average_scores


def predict(config_file, epoch, mode, augmentation_on, timeshift_ms, average_window_duration_ms):
    # load configuration file
    cfg = load_cfg_file(config_file)

    # init 
    num_classes = cfg.dataset.label.num_classes

    # load prediction model
    model = kws_load_model(cfg.general.save_dir, int(cfg.general.gpu_ids), epoch)
    net = model['prediction']['net']
    net.eval()

    # load label index 
    label_index = load_label_index(cfg.dataset.label.positive_label)

    # load data 
    data_pd = pd.read_csv(cfg.general.data_csv_path)
    data_pd_mode = data_pd[data_pd['mode'] == mode]
    data_file_list = data_pd_mode['file'].tolist()
    data_mode_list = data_pd_mode['mode'].tolist()
    data_label_list = data_pd_mode['label'].tolist()

    results_list = []
    preds = []
    labels = []
    for audio_idx in tqdm(range(len(data_file_list))):
        results_dict = {}
        results_dict['file'] = data_file_list[audio_idx]
        results_dict['mode'] = data_mode_list[audio_idx]
        results_dict['label'] = data_label_list[audio_idx]
        results_dict['label_idx'] = label_index[results_dict['label']]
        assert results_dict['mode']  == mode, "[ERROR:] Something wronge about mode, please check"

        # # debug
        # if results_dict['file'] != "/home/huanyuan/data/speech/kws/xiaoyu_dataset_03022018/XiaoYuDataset_10272020/xiaoyu/7276078M1_唤醒词_小鱼小鱼_女_中青年_是_0192.wav":
        #     continue

        pred, score = longterm_audio_predict(cfg, net, audio_idx, results_dict['file'], results_dict['mode'], results_dict['label'], results_dict['label_idx'], timeshift_ms, average_window_duration_ms)

        preds.append(pred)
        labels.append(results_dict['label_idx'])

        results_dict['result_idx'] = pred
        for classe_idx in range(num_classes):
            results_dict['prob_{}'.format(classe_idx)] = score[classe_idx]
        
        results_list.append(results_dict)

    # caltulate accuracy
    accuracy = float((np.array(preds) == np.array(labels)).astype(int).sum()) / float(len(labels))
    msg = 'epoch: {}, batch: {}, {}_accuracy: {:.4f}'.format(model['prediction']['epoch'], model['prediction']['batch'], mode, accuracy)
    print(msg)

    # out csv
    csv_data_pd = pd.DataFrame(results_list)
    csv_data_pd.to_csv(os.path.join(cfg.general.save_dir, 'infer_longterm_average_{}_augmentation_{}.csv'.format(mode, augmentation_on)), index=False, encoding="utf_8_sig")
    return accuracy


def main():
    """
    使用模型对音频文件进行测试，配置为 --input 中的 config 文件，当存在音频文件长度大于模型送入的音频文件长度时(1s\2s), 该脚本会通过滑窗的方式测试每一小段音频数据，计算连续 500ms(17帧) 音频的平均值结果，
    在得到的平均结果中对应label最小值作为最终结果
    该过程近似测试流程，可以作为参考
    """

    # default_mode = "training"
    # default_mode = "testing,validation,training"
    # default_mode = "testing,validation"
    default_mode = "validation"
    default_model_epoch = -1
    default_augmentation_on = False
    default_timeshift_ms = 30
    default_average_window_duration_ms = 500

    parser = argparse.ArgumentParser(description='Streamax KWS Infering Engine')
    # parser.add_argument('--input', type=str, default="/home/huanyuan/code/demo/Speech/KWS/config/kws/kws_config.py", help='config file')
    # parser.add_argument('--input', type=str, default="/home/huanyuan/code/demo/Speech/KWS/config/kws/kws_config_xiaoyu.py", help='config file')
    parser.add_argument('--input', type=str, default="/home/huanyuan/code/demo/Speech/KWS/config/kws/kws_config_xiaoyu_2.py", help='config file')
    parser.add_argument('--mode', type=str, default=default_mode)
    parser.add_argument('--epoch', type=str, default=default_model_epoch)
    parser.add_argument('--augmentation_on', type=bool, default=default_augmentation_on)
    parser.add_argument('--timeshift_ms', type=int, default=default_timeshift_ms)
    parser.add_argument('--average_window_duration_ms', type=int, default=default_average_window_duration_ms)
    args = parser.parse_args()

    mode_list = args.mode.strip().split(',')
    for mode_type in mode_list:
        predict(args.input, args.epoch, mode_type, args.augmentation_on, args.timeshift_ms, args.average_window_duration_ms)


if __name__ == "__main__":
    main()
