# ORL / AT&T 人脸数据库（已整理，可直接导入）

本目录的数据来自剑桥大学托管的 AT&T Laboratories Cambridge 官方存档。
共 **40 位不同人员、400 张真实人脸图片**，每人 10 张，原始格式为 92×112 像素的 8 位灰度 PGM。
人物使用匿名编号 `s01`～`s40`，对应原始目录 `s1`～`s40`，不代表真实姓名。

## 直接使用

在 Qt 程序中进入 **OpenCV 工作台 → PCA 人脸识别**，预处理选择
**已裁剪的单人人脸（如 ORL / AT&T）**，不需要 Haar XML。

| 用途 | 导入/选择的目录 | 人数 | 每人张数 | 总张数 | 原始图片编号 |
| --- | --- | ---: | ---: | ---: | --- |
| 训练人员库 | `data/att_faces/train` | 40 | 7 | 280 | 1～7 |
| 验证/调节参数 | `data/att_faces/validation` | 40 | 1 | 40 | 8 |
| 最终测试 | `data/att_faces/test` | 40 | 2 | 80 | 9～10 |

1. 点击“导入人员库目录”，选择 **train**，然后点击“训练 PCA 模型”。
2. 在“识别与逐人比对”载入例如 `test/s01/09.pgm`，点击“开始识别”，即可比对 40 位人员。
3. 在“独立测试集评估”先选择 **validation**，用于调整 PCA 维数或拒识阈值。
4. 参数确定后选择 **test**，记录最终结果并导出 CSV。

请准确选择 `train`、`validation` 或 `test`，不要把本目录或 `source` 当作人员库。
这三个集合中的 40 位人员相同，但照片不同；此划分用于已登记人员的识别评估，
不包含未登记人员，因此不能用它估计未知人员的拒识率。

## 文件与可复现性

```text
att_faces/
  train/s01/01.pgm ... 07.pgm
  ...
  train/s40/01.pgm ... 07.pgm
  validation/s01/08.pgm
  ...
  test/s01/09.pgm 10.pgm
  ...
  contact_sheet.jpg       # 40 人预览，每人使用第 1 张训练照片
  manifest.csv            # 400 张图片的集合、标签、原始路径、尺寸与 SHA-256
  dataset.json            # 来源、署名、原始压缩包校验值与划分说明
  source/
    att_faces.zip         # 官方原始压缩包，保留不改动
    facedatabase.html     # 下载时保存的官方说明网页
    ORIGINAL_README.txt   # 原压缩包中的 README
```

图片仅按确定规则分组及重命名，像素和文件字节均保持原始值。
整理脚本检查原压缩包 SHA-256、ZIP CRC、40×10 文件清单、PGM 尺寸与像素字节数、
400 张原始图片的唯一性。随后使用 OpenCV 检查全部图片能解码并通过系统的预处理步骤。

在项目根目录可以重复执行，已有且相同的文件会保留，内容不同的文件不会被覆盖：

```bash
python3 scripts/prepare_att_faces.py
```

原始压缩包大小：3,769,022 字节。
SHA-256：`509d763bd2276aa054d260d7f1000e8be212738134f2f83d8ff13bc2adf1c8ec`。

## 来源、署名和引用

- [官方数据集说明](https://www.cl.cam.ac.uk/research/dtg/attarchive/facedatabase.html)
- [官方 ZIP 下载](https://www.cl.cam.ac.uk/Research/DTG/attarchive/pub/data/att_faces.zip)
- 官方网页要求使用图片时注明 **AT&T Laboratories Cambridge**；原始 README 要求注明 **Olivetti Research Laboratory**。请在课程报告和演示中保留这两项署名。
- 参考文献：F. Samaria and A. Harter, “Parameterisation of a stochastic model for human face identification”, 2nd IEEE Workshop on Applications of Computer Vision, December 1994, Sarasota, Florida.

报告中可注明：**人脸数据来源：ORL / AT&T Database of Faces；图片由 Olivetti Research Laboratory / AT&T Laboratories Cambridge 提供。**
