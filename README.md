# 300ResourceBrowser
300英雄游戏资源文件浏览器 - 简易版本  [【下载地址】](../../releases)

![image](https://user-images.githubusercontent.com/111073265/184269582-218639e9-a187-4ad6-9560-533bd27ad3fc.png)

# 四种读取模式

### 读取游戏目录资源 
- 需要在【设置】中设置正确的游戏路径才能使用(可以自动搜索游戏路径)。
- 可以一次性读取游戏的4个资源包`Data.jmp` `Data1.jmp` `Data2.jmp` `Data3.jmp`，以及外部资源文件夹`asyncfile`。
- 右下角第6个视窗可以点击【校对列表】按钮，校对游戏资源是否有缺失的文件，点击【全部下载】可以将缺失的资源全部下载到`asyncfile`文件夹中。

![image](https://user-images.githubusercontent.com/111073265/184270344-9310d222-c7e8-4544-8e66-817fa97acf18.png) 

### 读取单个data资源
- 最基础的读取单个游戏资源包(.jmp文件)的功能。
- 在【设置】中设置【文件关联】后，可以直接双击资源包文件进行读取。

![image](https://user-images.githubusercontent.com/111073265/184271559-a62cfca9-198f-407e-a1f1-5321894c15a3.png)

### 读取本地目录资源
- 在【设置】中设置【导出路径】，所有按照【路径导出】的文件都会按照资源包内部的路径，保存在【导出路径】的相对路径中，然后通过这个读取本地已经导出的资源。
- 已经导出的资源会根据相对路径重新生成资源包内部的**补丁路径**。
- 右上角【折叠缺失文件列表】/【展开缺失文件列表】按钮，控制右侧的缺失文件视窗。
- 右侧缺失文件列表视窗，可以点击右上角的【校对列表】来校对本地资源是否有缺失文件，点击【全部下载】可以将缺失的资源全部下载到本地导出目录中。

![image](https://user-images.githubusercontent.com/111073265/184272375-e0b584e8-5b8e-404b-9297-6eddc995a366.png)

### 读取网络资源列表
- 连接300英雄官方服务器，直接获取最新的游戏资源列表。
- 可以选择部分文件下载到【导出路径】或者【单独路径】。
- 点击右上角【全部下载】，可以直接下载所有资源到本地【导出路径】。

![image](https://user-images.githubusercontent.com/111073265/184272795-b4175557-26ea-40b6-920e-757554fa55c1.png)

# 其他功能

### 快捷复制
- 双击选中项，可以快速复制选中项的补丁路径。

![image](https://user-images.githubusercontent.com/111073265/184276499-764b145d-410e-495e-b8ec-7210a06653a6.png)

### 略缩图预览器
- 选中图片文件(.bmp.png.jpg.dds.tga)时，会在右下角(默认)生成一个略缩图预览器。
- 预览器可以随意在软件内部拖动，双击或者单击右上角【×】键隐藏，点击其他图片文件时会再次出现，彻底关闭需要在【设置】里调整。
- 鼠标右键单击图片文件选中项，右键菜单中有【预览全图】，可以查看完整分辨率的图片。
- 在【设置】中，略缩图预览器有【缓读】和【直读】两种读取模式：
  - 【缓读】模式比较照顾配置性能较差的用户，读取图片会在后台线程里进行，不会影响到前台快速浏览文件。快速点击多个图片文件，只会显示最后一个图片文件的略缩图，因为前面的图片文件还没有读取完毕，就点击了下一张图片，新的图片读取会打断旧的图片读取。好处是无论图片多大都不会影响到前台快速浏览文件，坏处是不能过目每一张图片。
  - 【直读】模式没有使用后台线程，而是直接使用前台主线程读取图片，读取完毕显示略缩图后，才能切换到下一个选中项。如果配置性能足够，那么几乎不会有滞留感，但如果性能较差，切换多张图片时，就会明显感觉到卡顿。好处是读取图片不会有【缓读】模式下的预留延迟，性能好就会比【缓读】舒服。

![image](https://user-images.githubusercontent.com/111073265/184273007-579f4f53-e3b4-445e-aa70-34671eb09dbb.png)

### 文本预览
- 选中文本文件(.txt.ini.lua.xml.json.fx.psh.string)时，右键菜单中可以选择【预览文本】(只读)。

![image](https://user-images.githubusercontent.com/111073265/184275096-60397b05-d1e6-4bc7-a25b-0c4ae9400920.png)

### 单独导出
- 除了【路径导出】外，还可以将文件直接导出到一个单独的目录里，不会按照原本的内部资源路径一层一层的创建文件路径，而是只导出文件本身。

### 文件信息
- 左上角提供了选中项的文件信息，不同的文件所显示的信息区域数量不同：
  - 数据包中压缩的内部资源文件，会显示【补丁路径】【索引位置】【文件大小】【压缩大小】；
  - 本地存在的资源文件，会显示【文件路径】【补丁路径】【文件大小】；
  - 网络资源文件，仅显示【补丁路径】；
- 【文件路径】的【…】可以快速跳转到文件所在目录，【C】(copypath)则是复制文件路径。
- 【补丁路径】的【F】(filename)是复制文件名，【C】(copypath)是复制补丁路径。+

![image](https://user-images.githubusercontent.com/111073265/184276026-5816e769-509b-4fe1-9ddb-2d94759e185d.png)
![image](https://user-images.githubusercontent.com/111073265/184276115-1f4f827e-6ccb-40e2-a96e-e1bd64d4439b.png)
![image](https://user-images.githubusercontent.com/111073265/184276174-2d25bcb8-5c7c-4767-b945-6905b395f81f.png)

# 代码开发环境配置

### 开源代码获取方式：
- [git项目页面](../../../300ResourceBrowser.git)直接打包获取
- 软件右上角logo部分鼠标右键单击即可获取开源代码

### 开发环境
- Windows系统(因为用到了大量Windows API)
- QT 5.14.2
- MSVC 2017 32bit static
- C++ 11

# 项目依赖
- 因为QT自身图片库，没有对dds和tga的支持，于是引入了[r-lyeh](https://github.com/r-lyeh)的开源库[spot](https://github.com/r-lyeh-archived/spot.git)。
- 引入的部分为文件[img_decode.h](img_decode.h)，因为本项目中只需要用到tga和dds的解码部分，所以我对spot库源码进行了提炼，只保留了这一部分功能，并且对此进行了类封装以对接项目的使用。
- spot库的[许可证](https://github.com/r-lyeh-archived/spot/blob/171c208314d413330973cfefe5d14b7908621f42/LICENSE)。

# 许可证声明
#### 本项目完全开源，不设任何限制
