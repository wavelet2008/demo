import argparse
import os 
import sys

sys.path.insert(0, '/home/huanyuan/code/demo/Speech/KWS')
from utils.train_tools import *

def analysis_data_distribution(config_file):
    # load configuration file
    cfg = load_cfg_file(config_file)

    # load data 
    data_pd = pd.read_csv(cfg.general.data_csv_path)
    label_list = cfg.dataset.label.label_list

    # output_dir 
    output_dir = os.path.join(os.path.dirname(cfg.general.data_csv_path), 'data_distribution')
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    data_number_dict = {}
    avaliable_mode = ['training', 'validation', 'testing']
    for mode in avaliable_mode:
        data_number_dict[mode] = {}
        mode_pd = data_pd[data_pd['mode'] == mode]
        for label in label_list:
            label_pd = mode_pd[mode_pd['label'] == label]
            data_number_dict[mode][label] = label_pd.shape[0]
    
    print(data_number_dict)
    data_number_pd = pd.DataFrame(data_number_dict)
    data_number_pd.to_csv(os.path.join(output_dir, 'data_distribution.csv'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config_file', type=str,
                        default="/home/huanyuan/code/demo/Speech/KWS/config/kws/kws_config_xiaoyu_2.py")
    args = parser.parse_args()

    analysis_data_distribution(args.config_file)


if __name__ == "__main__":
    main()