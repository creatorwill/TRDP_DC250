#if !defined(AFX_Csharememoryclient_H__A1E4BFC9_DD34_4053_BC3B_4A310B482F1F__INCLUDED_)
#define AFX_Csharememoryclient_H__A1E4BFC9_DD34_4053_BC3B_4A310B482F1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif
 

/* 共享内存分配类型 */

/**
 * strType : 共享内存中类型标志 
 *          "comm-record" : 公共端口:   0~9999，记录内容: 10000~19999;
 *          "tcn-bcn"     : 过程数据:   0~19999
 *          "fault1"      : 一二级故障: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *          "fault3"      : 三级故障:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *          "other"       : 编号 预留
 * data    : 存储数据 
 */  
typedef struct _ST_DATA_INFO {
	char     strType[20]; 
	unsigned data[20000];   // 数据
} st_data_info;

/**
 *   memory_data[0] :"comm-record" : 公共端口:   0~9999，记录内容: 10000~19999;
 *   memory_data[1] :"tcn-bcn"     : 过程数据:   0~19999
 *   memory_data[2] :"fault1"      : 一二级故障: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *   memory_data[3] :"fault3"      : 三级故障:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *   memory_data[4] :"other"       : 编号 预留
 *   memory_data[5] :"mvb"         : 存放 mvb数据 , 供 trdp进程 读取
 */
typedef struct _ST_SHARED_MEMORY{
	BOOL bFlag;                  // 是否已存在共享内存,
	BOOL isValid[6];                  // 是否已存在共享内存,
	st_data_info memory_data[6]; // 0~4:存放接收的TRDP数据 , 5: 存放 mvb数据
} st_shared_memory;

class Csharememoryclient 
{
// Construction
public:
	Csharememoryclient();	
	~Csharememoryclient();

// Attributes
public:
	LPBYTE MvbToTrdp;
	LPBYTE TrdpToMvb;
	
	HANDLE m_hFileMvbToTrdpMap;
	HANDLE m_hFileTrdpToMvbMap;

	st_shared_memory *memory_trdp_mvb; // TRDP & MVB，共享数据

	UINT initsharememory();
	WriteTrdpDatatoMvb(st_data_info *buf, unsigned index);
	ReadMvbDatatoTrdp(st_data_info *buf,  unsigned index = 5); //index = 5  共享mvb数据 , 供 trdp进程 读取

protected:

private:

};

#endif
