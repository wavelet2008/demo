#include "inference_detection_openvino.hpp"

#include "glog/logging.h"
#include "gflags/gflags.h"

#ifdef WIN32
#ifndef AIP_API
#define AIP_API
#endif // AIP_API
#else
#ifndef AIP_API
#define AIP_API __attribute__((visibility("default")))
#endif // AIP_API
#endif

namespace inference_openvino
{

  InferenceEngine::Blob::Ptr wrapMat2Blob(const cv::Mat &mat) {
	size_t channels = mat.channels();
	size_t height = mat.size().height;
	size_t width = mat.size().width;

	size_t strideH = mat.step.buf[0];
	size_t strideW = mat.step.buf[1];

	bool is_dense =
	  strideW == channels &&
	  strideH == channels * width;

	if (!is_dense) THROW_IE_EXCEPTION
	  << "Doesn't support conversion from not dense cv::Mat";

	InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::U8,
	  { 1, channels, height, width },
	  InferenceEngine::Layout::NHWC);

	return InferenceEngine::make_shared_blob<uint8_t>(tDesc, mat.data);
  }

  AIP_API rmInferenceDetectionOpenvino::rmInferenceDetectionOpenvino(const std::string strModelPath)
  {
	m_InferenceOptions = INFERENCE_OPTIONS_S();
	m_strModelPath = strModelPath;
  }

  AIP_API rmInferenceDetectionOpenvino::rmInferenceDetectionOpenvino(const std::string strModelPath, const INFERENCE_OPTIONS_S &InferenceOptions)
  {
	m_InferenceOptions = INFERENCE_OPTIONS_S(InferenceOptions);
	m_strModelPath = strModelPath;
  }

  AIP_API rmInferenceDetectionOpenvino::~rmInferenceDetectionOpenvino()
  {
  }

  AIP_API int rmInferenceDetectionOpenvino::Init()
  {
	LOG(INFO) << "Start init.";

	// 0. Check input
	if (m_InferenceOptions.OpenvinoOptions.strDeviceType != "CPU" &&
	  m_InferenceOptions.OpenvinoOptions.strDeviceType != "GPU" &&
	  m_InferenceOptions.OpenvinoOptions.strDeviceType != "MULTI:CPU,GPU") {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", DeviceName only support for ['CPU'/'GPU'/'MULTI:CPU,GPU'], can not be "
		<< m_InferenceOptions.OpenvinoOptions.strDeviceType;
	  return -1;
	}

	if (m_InferenceOptions.OpenvinoOptions.strDeviceType == "CPU" &&
	  (m_InferenceOptions.OpenvinoOptions.u32ThreadNum <= 0 ||
		m_InferenceOptions.OpenvinoOptions.u32ThreadNum > 32)) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", ThreadNum only support for 1 ~ 32, can not be " << m_InferenceOptions.OpenvinoOptions.u32ThreadNum;
	  return -1;
	}

	if (m_strModelPath.find(".xml") == m_strModelPath.npos) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", ModelPath type must be .xml";
	  return -1;
	}

	// 1. Load inference engine
	LOG(INFO) << "Loading inference engine";
	InferenceEngine::Core Ie;
	Ie.GetVersions(m_InferenceOptions.OpenvinoOptions.strDeviceType);
	LOG(INFO) << "Device Type: \n\t" << m_InferenceOptions.OpenvinoOptions.strDeviceType;

	if (m_InferenceOptions.OpenvinoOptions.strDeviceType == "CPU") {
	  std::map<std::string, std::map<std::string, std::string>> nConfig;
	  nConfig[m_InferenceOptions.OpenvinoOptions.strDeviceType] = {};
	  std::map<std::string, std::string> &nDeviceConfig = nConfig.at(m_InferenceOptions.OpenvinoOptions.strDeviceType);
	  nDeviceConfig[CONFIG_KEY(CPU_THREADS_NUM)] = std::to_string(m_InferenceOptions.OpenvinoOptions.u32ThreadNum);
	  LOG(INFO) << "CPU Thread Number: \n\t" << m_InferenceOptions.OpenvinoOptions.u32ThreadNum;

	  for (auto &&item : nConfig) {
		Ie.SetConfig(item.second, item.first);
	  }
	}

	// 2. Read IR Generated by ModelOptimizer (.xml and .bin files)
	std::string strBinFileName = m_strModelPath.substr(0, m_strModelPath.rfind('.')) + ".bin";
	LOG(INFO) << "Loading network files: \n\t" << m_strModelPath << "\n\t" << strBinFileName;
	m_Network = Ie.ReadNetwork(m_strModelPath);
	LOG(INFO) << "Batch size is " << std::to_string(m_Network.getBatchSize());

	// 3. Prepare input blobs
	LOG(INFO) << "Preparing input blobs";
	InferenceEngine::InputsDataMap InputInfo(m_Network.getInputsInfo());
	if (InputInfo.size() != 1) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", FaceSsdOpenvino has only one input but got " << InputInfo.size();
	  return -1;
	}
	m_InputInfo = InputInfo.begin()->second;
	m_InputInfo->setPrecision(InferenceEngine::Precision::U8);
	m_InputInfo->setLayout(InferenceEngine::Layout::NCHW);
	/* Mark input as resizable by setting of a resize algorithm.
		* In this case we will be able to set an input blob of any shape to an infer request.
		* Resize and layout conversions are executed automatically during inference */
	// conflict in multi thread concurrent inference 
	//m_InputInfo->getPreProcess().setResizeAlgorithm(InferenceEngine::RESIZE_BILINEAR);
	m_strInputName = InputInfo.begin()->first;

	// Check input size
	m_InputDims = m_InputInfo->getTensorDesc().getDims();
	if (m_InputDims.size() != 4) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", FaceSsdOpenvino input dim should be 4 but got " << m_InputDims.size();
	  return -1;
	}
	if (m_InputDims[1] != 1 && m_InputDims[1] != 3) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", FaceSsdOpenvino input channels should be 1 or 3 but got " << m_InputDims[1];
	  return -1;
	}

	// 4. Prepare output blobs
	LOG(INFO) << "Preparing output blobs";
	InferenceEngine::OutputsDataMap OutputInfo(m_Network.getOutputsInfo());
	// if (OutputInfo.size() != 1) {
	// 	LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
	// 	  				 << ", FaceSsdOpenvino has only one output but got " << OutputInfo.size() << std::endl;
  //   return -1;
  // }
	m_OutputInfo = OutputInfo.begin()->second;
	if (m_OutputInfo == nullptr) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", FaceSsdOpenvino do not have a output" << std::endl;
	  return -1;
	}

	m_OutputInfo = OutputInfo.begin()->second;
	m_strOutputName = m_OutputInfo->getName();
	m_OutputInfo->setPrecision(InferenceEngine::Precision::FP32);
	CHECK_NOTNULL(m_OutputInfo);

	// Check output size
	m_OutputDims = m_OutputInfo->getTensorDesc().getDims();
	const size_t u32ObjectSize = m_OutputDims[3];

	if (u32ObjectSize != 7) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", Output item should have 7 as a last dimension";
	  return -1;
	}

	if (m_OutputDims.size() != 4) {
	  LOG(ERROR) << "ERROR, func: " << __FUNCTION__ << ", line: " << __LINE__
		<< ", Incorrect output dimensions for SSD model";
	  return -1;
	}

	// 5. Loading model to the device
	LOG(INFO) << "Loading model to the device";
	//InferenceEngine::ExecutableNetwork ExecutableNetwork = Ie.LoadNetwork(
	m_ExecutableNetwork = Ie.LoadNetwork(
	  m_Network,
	  m_InferenceOptions.OpenvinoOptions.strDeviceType);

	//// 6. Create infer request
	//LOG(INFO) << "Create infer request";
	//m_InferRrequest = ExecutableNetwork.CreateInferRequest();

	LOG(INFO) << "End init.";
	return 0;
  }

  AIP_API int rmInferenceDetectionOpenvino::Detect(const cv::Mat &cvMatImage, std::vector<OBJECT_INFO_S> *pstnObject)
  {
	CHECK_NOTNULL(pstnObject);

	LOG(INFO) << "Start detect.";

	const size_t u32InputC = m_InputDims[1];
	const size_t u32InputH = m_InputDims[2];
	const size_t u32InputW = m_InputDims[3];
	const size_t u32MaxProposalCount = m_OutputDims[2];
	const size_t u32ObjectSize = m_OutputDims[3];

	// Prepare input
	int s32ImageWidth = cvMatImage.cols;
	int s32ImageHeight = cvMatImage.rows;

	// 1. Create infer request
	LOG(INFO) << "Create infer request";
	InferenceEngine::InferRequest m_InferRrequest = m_ExecutableNetwork.CreateInferRequest();

	// 2. Prepare input
	//cv::Mat cvMatImageResized;
	//cv::resize(cvMatImage, cvMatImageResized, cv::Size(static_cast<int>(m_InputInfo->getTensorDesc().getDims()[3]), static_cast<int>(m_InputInfo->getTensorDesc().getDims()[2])));

	//InferenceEngine::Blob::Ptr pstImageInput = wrapMat2Blob(cvMatImageResized);
	//m_InferRrequest.SetBlob(m_strInputName, pstImageInput);

	// resize image
	cv::Mat cvMatImageResized(cvMatImage);
	cv::resize(cvMatImage, cvMatImageResized, cv::Size(static_cast<int>(m_InputInfo->getTensorDesc().getDims()[3]), static_cast<int>(m_InputInfo->getTensorDesc().getDims()[2])));

	InferenceEngine::Blob::Ptr pstImageInput = m_InferRrequest.GetBlob(m_strInputName);
	// Filling input tensor with images. First b channel, then g and r channels
	InferenceEngine::MemoryBlob::Ptr pstMemortImage = InferenceEngine::as<InferenceEngine::MemoryBlob>(pstImageInput);
	if (!pstMemortImage) {
		LOG(ERROR) << "We expect image blob to be inherited from MemoryBlob, but by fact we were not able "
									"to cast imageInput to MemoryBlob";
		return -1;
	}
	// locked memory holder should be alive all time while access to its buffer happens
	auto InputHolder = pstMemortImage->wmap();
	unsigned char *pstucData = InputHolder.as<unsigned char *>();

	/** Iterate over all pixel in image (b,g,r) **/
	for (size_t u32Pid = 0; u32Pid < u32InputH * u32InputW; u32Pid++) {
		/** Iterate over all channels **/
		for (size_t u32Ch = 0; u32Ch < u32InputC; ++u32Ch) {
			/**          [images stride + channels stride + pixel id ] all in bytes            **/
			pstucData[u32Ch * u32InputH * u32InputW + u32Pid] = cvMatImageResized.data[u32Pid * u32InputC + u32Ch];
		}
	}

   // 3. Do inference
	LOG(INFO) << "Start inference";
	m_InferRrequest.Infer();

	// 4. Process output
	LOG(INFO) << "Processing output blobs";
	const InferenceEngine::Blob::Ptr pstOutputBlob = m_InferRrequest.GetBlob(m_strOutputName);
	InferenceEngine::MemoryBlob::CPtr pstMemoryOutput = InferenceEngine::as<InferenceEngine::MemoryBlob>(pstOutputBlob);
	if (!pstMemoryOutput) {
	  LOG(ERROR) << "We expect output to be inherited from MemoryBlob, "
		"but by fact we were not able to cast output to MemoryBlob";
	  return -1;
	}

	// locked memory holder should be alive all time while access to its buffer happens
	auto OutputHolder = pstMemoryOutput->rmap();
	const float *pstFeatures = OutputHolder.as<const InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type *>();

	/* Each detection has image_id that denotes processed image */
	for (size_t s32CurProposal = 0; s32CurProposal < u32MaxProposalCount; s32CurProposal++) {

	  float f32Confidence = pstFeatures[s32CurProposal * u32ObjectSize + 2];
	  auto label = static_cast<int>(pstFeatures[s32CurProposal * u32ObjectSize + 1]);
	  auto xmin = static_cast<int>(pstFeatures[s32CurProposal * u32ObjectSize + 3] * s32ImageWidth);
	  auto ymin = static_cast<int>(pstFeatures[s32CurProposal * u32ObjectSize + 4] * s32ImageHeight);
	  auto xmax = static_cast<int>(pstFeatures[s32CurProposal * u32ObjectSize + 5] * s32ImageWidth);
	  auto ymax = static_cast<int>(pstFeatures[s32CurProposal * u32ObjectSize + 6] * s32ImageHeight);
	  xmin = std::max(0, xmin);
	  xmin = std::min(xmin, s32ImageWidth - 1);
	  ymin = std::max(0, ymin);
	  ymin = std::min(ymin, s32ImageHeight - 1);
	  xmax = std::max(0, xmax);
	  xmax = std::min(xmax, s32ImageWidth - 1);
	  ymax = std::max(0, ymax);
	  ymax = std::min(ymax, s32ImageHeight - 1);

	  if (f32Confidence > m_InferenceOptions.f64Threshold)
	  {
		/** Drawing only objects with >50% probability **/
		OBJECT_INFO_S Object;
		Object.strClassName = m_InferenceOptions.nClassName[label];
		Object.f32Score = f32Confidence;
		Object.cvRectLocation.x = xmin;
		Object.cvRectLocation.y = ymin;
		Object.cvRectLocation.width = xmax - xmin;
		Object.cvRectLocation.height = ymax - ymin;
		pstnObject->push_back(Object);
	  }
	}

	LOG(INFO) << "end detect.";
	return 0;
  }

} // namespace inference_openvino