#if !defined(AFX_Csharememoryclient_H__A1E4BFC9_DD34_4053_BC3B_4A310B482F1F__INCLUDED_)
#define AFX_Csharememoryclient_H__A1E4BFC9_DD34_4053_BC3B_4A310B482F1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif
 

/* �����ڴ�������� */

/**
 * strType : �����ڴ������ͱ�־ 
 *          "comm-record" : �����˿�:   0~9999����¼����: 10000~19999;
 *          "tcn-bcn"     : ��������:   0~19999
 *          "fault1"      : һ��������: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *          "fault3"      : ��������:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *          "other"       : ��� Ԥ��
 * data    : �洢���� 
 */  
typedef struct _ST_DATA_INFO {
	char     strType[20]; 
	unsigned data[20000];   // ����
} st_data_info;

/**
 *   memory_data[0] :"comm-record" : �����˿�:   0~9999����¼����: 10000~19999;
 *   memory_data[1] :"tcn-bcn"     : ��������:   0~19999
 *   memory_data[2] :"fault1"      : һ��������: c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *   memory_data[3] :"fault3"      : ��������:   c1 :0~4999, c2:5000~9999, c3:10000~14999, c4:15000~19999
 *   memory_data[4] :"other"       : ��� Ԥ��
 *   memory_data[5] :"mvb"         : ��� mvb���� , �� trdp���� ��ȡ
 */
typedef struct _ST_SHARED_MEMORY{
	BOOL bFlag;                  // �Ƿ��Ѵ��ڹ����ڴ�,
	BOOL isValid[6];                  // �Ƿ��Ѵ��ڹ����ڴ�,
	st_data_info memory_data[6]; // 0~4:��Ž��յ�TRDP���� , 5: ��� mvb����
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

	st_shared_memory *memory_trdp_mvb; // TRDP & MVB����������

	UINT initsharememory();
	WriteTrdpDatatoMvb(st_data_info *buf, unsigned index);
	ReadMvbDatatoTrdp(st_data_info *buf,  unsigned index = 5); //index = 5  ����mvb���� , �� trdp���� ��ȡ

protected:

private:

};

#endif
