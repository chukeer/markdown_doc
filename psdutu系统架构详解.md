# psdutu系统架构详解

## 部署环境
-------------------
* 自然结果周级挖掘：spider@cq01-spi-pctest5.vm.baidu.com:/home/spider/handy/ps_arch/ui_arch* 自然结果天级挖掘：spider@cq01-spi-pctest5.vm.baidu.com:/home/spider/handy/ps_arch/ui_daily* 视频：spider@cq01-spi-pctest5.vm.baidu.com:/home/spider/handy/ps_arch/video_pic
## 流程详解
----------------------
### VIP周级
1. 主图提取，推送spider抓取——part-rank

	概述：挖掘VIP库网页，提取主图链接，并将主图链接交给spider抓取，并写入图库ccdb  
	步骤详解：
	  
	**pro2_dataminer_bvc**	
	功能：用dataminer挖掘ccdb中文大库的VIP网页，运行环境为bvc  
	输出：产出结果为每个网页的相关信息，包括objurl，文本，以及基础特  
	**pro3.2_merge_data**	
	功能：merge两份离线特征：图片宽高和站点内重复度  
	输出：merge后的特征文件  
	**pro4_offline_calc**	
	功能：计算主图链接  	
	输出：网页内主图信息	
	**pro5_extract_obj_for_spider**  	
	功能：提取pro4中的obj并去重，distcp到指定地址，供spider抓取  
	**psdutu2img**  
	功能：将pro2的产出信息灌ccdb
2. 图片特征计算——part-ccdb

	概述：获取图片信息，计算图片内容特征，特征merge  
	步骤详解：  
	<font color='red'>以下为增量计算</font>  
	<b>part2-work1_pre	</b>  
	功能：从现有文件里找出已经获取了图片大小的数据  
	输出：1、已经获取了大小的obj信息；2、没有获取大小的obj列表  
	<b>part2-work1</b>  
	功能：将未获取大小的obj从ccdb img表获取大小  
	输出：obj大小信息  
	<b>part2-work2_pre</b>  
	功能：从现有特征文件里找到已经计算图片内容特征的contsign信息  
	输出：1、已经计算过图片特征的contsign信息；2、没计算特征的contsign列表  
	<b>part2-work2</b>  
	功能：从ccdb tn表获取原图  
	输出：未计算特征的contsign对应的原图，两份格式，一份原始数据，还有一份用来计算原图特征  
	<b>mmt_process</b>  
	功能：计算原图特征，运行环境bvc  
	输出：原图特征，格式sequencefile，key是随机值，value是mcpack  
	<b>part2-work5</b>  
	功能：将原图特征文件格式化，主要是更改key为contsign  
	输出：原图特征  
	<b>part2-work17</b>  
	功能：将原图和特征进行merge，并将特征抽取成行文本，方便后续计算  
	输出：特征文件（行文本），原图文件（二进制）  
	<b>part2-work18</b>  
	功能：将计算出的特征文件加入特征库  
	输出：特征库  
	<font color='red'>以下为全量计算</font>  
	<b>part2-work19</b>  
	功能：特征merge  
	输出：以obj为key的特征文件  
	<b>part2-work20</b>  
	功能：特征merge  
	输出：以furl为key的特征文件  
	<b>sex_rec</b>  
	功能：计算高级色情特征（0/1特征）  
	输出：色情特征  
	<b>part2-work21</b>  
	功能：特征merge  
	输出：merge了色情特征的最终结果  
	<b>part2-work22</b>  
	功能：过滤video数据  
	输出：过滤video数据后的最终结果  
3. 压缩灌库生成索引——part-tn

	概述：将part-ccdb产出的原图进行压缩灌库，并生成给ps的索引文件  
	步骤详解：  
	<font color='red'>以下为增量计算</font>  
	<b>filter_contsign</b>  
	功能：过滤已经灌库的数据，获取需要增量计算的contsign  
	输出：增量contsign（二进制）  
	<b>extract_add_origin</b>  
	功能：获取增量计算的原图  
	输出：原图数据（二进制）  
	<b>compress</b>  
	功能：原图压缩  
	输出：缩略图（二进制）  
	<b>collector_ps</b>  
	功能：灌库  
	<font color='red'>以下为全量计算</font>  
	<b>chktn</b>  
	功能：检查part-ccdb中的全量contsign，是否在mola中  
	输出：在mola中的contsign列表  
	<b>gen_filtered_contsign</b>  
	功能：根据在mola中的contsign过滤part2-work22的结果  
	输出：待建索引的数据（行文本）  
	<b>gen_filtered_index</b>  
	功能：生成索引文件  
	输出：索引文件（二进制）  

### 自然结果天级挖掘
1. 主图提取，推送spider抓取——part-rank

	概述：和VIP挖掘略有不同，通过URL挖掘网页包，且进行了一些过滤逻辑操作  
	步骤详解：  
	<b>pro1.5_distcp_url</b>  
	功能：拷贝当天建库的wdna和wp的URL列表  
	输出：URL列表   
	<b>pro4_filter_click</b>  
	功能：拷贝点击展现URL，并过滤（1. 根据点展阈值过滤；2. 根据天级建库索引过滤）  
	输出：天级建库URL列表  
	<b>pro5_dataminer</b>  
	功能：根据URL seek网页包，提取网页信息，和VIP Pro2相同  
	<b>pro6_merge_data</b>  
	功能：同VIP pro3.2  
	<b>pro7_offline_calc</b>  
	功能：同VIP pro4  
	<b>pro8_gather_index_undone</b>  
	功能：将pro7产出的raw文件和历史raw文件（只保留3天）合并并过滤，得到待建库的raw文件  
	输出：待建库的raw文件  
2. 同VIP
3. 同VIP

### 视频周级
1. VIP+SE列表获取

	功能：获取全库列表，标记库种（Layer）  
	输出：URL列表+Layer

2. Url归一化

	功能：在1的基础上根据归一化策略，得到更多的URL  
	输出：URL列表+Layer

3. merge video文件

	功能：和video文件求交，得到大库列表中的待建库数据  
	输出：待建库数据

4. 生成索引

	功能：对VIP+SE数据建库  
	输出：索引（二进制）

### 视频天级
历史原因导致这一段逻辑比较恶心  
背景：  
video缩略图该尺寸导致我们不能直接用他们的数据，因此要求他们将新增数据为我们压一份新尺寸，而新尺寸数据他们只提供了obj和contsign，并没有提供furl，因此无法增量建库（建库需要furl+obj+contsign+duration+intro），采用的方法是拿新尺寸的增量数据和全量老数据求交，得到建库需要的信息  
大致逻辑： 
 
1. 获取video新尺寸增量数据，从video的机器直接拷贝  
2. 和老的全量数据求教，得到建库需要信息  
3. 生成索引

## ps索引灌库
-------------------
这部分由dew负责写数据到mola，我们按指定的格式提供，distcp到指定的目录，并通过mis触发灌库任务自动执行  
为此我们需要提供三份数据，这里以自然结果天级建库举例，以下文件均在hdfs上

1. 索引数据，假设distcp到的目录为/xxx/daily，文件名为data_for_mola.xx（若干个）（必须以data_for_mola为前缀，shit！）
2. ready文件，命名为/xxx/daily_ready，内容为行文本，每行为date daily/data_for_mola.xx，也就是说每行对应一个文件名和它的时间戳（md5值也可       以，但是时间戳比较容易，格式可以自己定义）
3. md5文件，命名为/xxx/daily_ready.md5，内容为md5value daily_ready，md5value为具体的md5值

基本原理就是：  
每次daily目录更新后，同时更新ready文件和md5文件，mis系统检测到ready文件有更新就会触发daily目录关联的一个任务，该任务就是dew的灌库任务  
目前有4个灌库任务，分别是自然结果周级，自然结果天级，视频周级，视频天级，其中自然结果的资源号是2058，视频是2666  