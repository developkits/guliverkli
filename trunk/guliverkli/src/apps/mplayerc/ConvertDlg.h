#pragma once
#include "afxcmn.h"
#include "..\..\DSUtil\DSMPropertyBag.h"

class CFilterTreeCtrl : public CTreeCtrl
{
public:
	CFilterTreeCtrl();

protected:
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	virtual void PreSubclassWindow();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);
};

// CConvertDlg dialog

class CConvertDlg : public CResizableDialog
{
public:
	class CTreeItem
	{
	protected:
		CTreeCtrl& m_tree;
		HTREEITEM m_hTI;

	public: 
		CTreeItem(CTreeCtrl& tree, HTREEITEM hTIParent);
		virtual ~CTreeItem();
		virtual void Update() {}
		virtual bool ToolTip(CString& str) {return false;}
		void SetLabel(LPCTSTR label);
		void SetImage(int nImage, int nSelectedImage);
		operator HTREEITEM() {return m_hTI;}
	};
	
	class CTreeItemFilter : public CTreeItem 
	{
	public:
		CComPtr<IBaseFilter> m_pBF;
		CTreeItemFilter(IBaseFilter* pBF, CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
	};

	class CTreeItemFile : public CTreeItemFilter
	{
	public:
		CString m_fn;
		CTreeItemFile(CString fn, IBaseFilter* pBF, CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
		bool ToolTip(CString& str);
	};

	class CTreeItemPin : public CTreeItem
	{
	public:
		CComPtr<IPin> m_pPin;
		CTreeItemPin(IPin* pPin, CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
		bool ToolTip(CString& str);
		bool IsConnected();
	};

	class CTreeItemResourceFolder : public CTreeItem
	{
	public: 
		CTreeItemResourceFolder(CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
		bool ToolTip(CString& str);
	};

	class CTreeItemResource : public CTreeItem
	{
	public: 
		CDSMResource m_res;
		CTreeItemResource(const CDSMResource& res, CTreeCtrl& tree, HTREEITEM hTIParent);
		~CTreeItemResource();
		void Update();
		bool ToolTip(CString& str);

		static CCritSec m_csGlobalRes;
		static CAtlMap<DWORD, CDSMResource*> m_GlobalRes;
	};

	class CTreeItemChapterFolder : public CTreeItem
	{
	public: 
		CTreeItemChapterFolder(CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
	};

	class CTreeItemChapter : public CTreeItem
	{
	public: 
		CDSMChapter m_chap;
		CTreeItemChapter(const CDSMChapter& chap, CTreeCtrl& tree, HTREEITEM hTIParent);
		void Update();
	};

private:
	DWORD m_dwRegister;
	CComPtr<ICaptureGraphBuilder2> m_pCGB;
	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IBaseFilter> m_pMux;
	CComQIPtr<IMediaControl> m_pMC;
	CComQIPtr<IMediaEventEx> m_pME;
	CComQIPtr<IMediaSeeking> m_pMS;

	CString m_title;
	UINT m_nIDEventStatus;

	CBitmap m_streamtypesbm;
	CImageList m_streamtypes;

	CList<CTreeItem*> m_pTIs;

	void AddFile(CString fn);
	void AddFilter(HTREEITEM hTI, IBaseFilter* pBF);
	void DeleteFilter(IBaseFilter* pBF);
	void DeleteItem(HTREEITEM hTI);
	void DeleteChildren(HTREEITEM hTI);

	HTREEITEM HitTest(CPoint& sp, CPoint& cp);

	void ShowPopup(CPoint p);
	void ShowFilePopup(HTREEITEM hTI, CPoint p);
	void ShowPinPopup(HTREEITEM hTI, CPoint p);
	void ShowResourceFolderPopup(HTREEITEM hTI, CPoint p);
	void ShowResourcePopup(HTREEITEM hTI, CPoint p);
	void ShowChapterFolderPopup(HTREEITEM hTI, CPoint p);
	void ShowChapterPopup(HTREEITEM hTI, CPoint p);

	bool EditProperties(IDSMPropertyBag* pPB);
	bool EditResource(CTreeItemResource* t);
	bool EditChapter(CTreeItemChapter* t);

public:
	CConvertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConvertDlg();

// Dialog Data
	enum { IDD = IDD_CONVERTDIALOG };
	CFilterTreeCtrl m_tree;
	CString m_fn;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnNMClickTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton4(CCmdUI* pCmdUI);
};
