#pragma once


// CUrlInputDlg �Ի���

class CUrlInputDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CUrlInputDlg)

public:
	CUrlInputDlg(CString Title = TEXT(""), CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CUrlInputDlg();

// �Ի�������
	enum { IDD = IDD_URL_INPUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_Title;
	CString m_Url;
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
};
