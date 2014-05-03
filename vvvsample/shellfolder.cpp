//
// (C) Copyright by Victor Derks
//
// See README.TXT for the details of the software licence.
//
#include "stdafx.h"
#include "../include/shellfolderimpl.h"
#include "../include/browserframeoptionsimpl.h"
#include "../include/itemnamelimitsimpl.h"
#include "../include/strutil.h"
#include "../include/queryinfo.h"
#include "../include/cfhdrop.h"
#include "../include/menu.h"
#include "shellfolderclsid.h"
#include "shellfolderviewcb.h"
#include "shellfolderdataobject.h"
#include "enumidlist.h"
#include "vvvitem.h"
#include "vvvfile.h"
#include "columns.h"
#include "vvvpropertysheet.h"
#include "resource.h"

// Defines for the item context menu.
const UINT ID_DFM_CMD_OPEN = 0;

class ATL_NO_VTABLE CShellFolder :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CShellFolder, &__uuidof(CShellFolder)>,
    public IShellFolderImpl<CShellFolder, CVVVItem>,
    public IBrowserFrameOptionsImpl,
    public IItemNameLimitsImpl<CShellFolder, CVVVItem>
{
public:
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw()
    {
        return IShellFolderImpl<CShellFolder, CVVVItem>::UpdateRegistry(
            bRegister, IDR_SHELLFOLDER,
            L"VVV Sample ShellFolder ShellExtension ", wszVVVFileRootExt, IDS_SHELLFOLDER_TYPE);
    }

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CShellFolder)
        COM_INTERFACE_ENTRY2(IPersist, IPersistFolder2)
        COM_INTERFACE_ENTRY(IPersistFolder)
        COM_INTERFACE_ENTRY(IPersistFolder2)
        COM_INTERFACE_ENTRY(IPersistFolder3)
        COM_INTERFACE_ENTRY(IPersistIDList)
        COM_INTERFACE_ENTRY(IShellFolder)  // included in this sample for backwards (win9x) compatiblity.
        COM_INTERFACE_ENTRY(IShellFolder2)
        COM_INTERFACE_ENTRY(IShellDetails) // included in this sample for backwards (win9x) compatiblity.
        COM_INTERFACE_ENTRY(IBrowserFrameOptions)
        COM_INTERFACE_ENTRY(IShellIcon)
        COM_INTERFACE_ENTRY(IItemNameLimits)
        COM_INTERFACE_ENTRY(IDropTarget)   // enable drag and drop support.
        COM_INTERFACE_ENTRY(IObjectWithFolderEnumMode) // used by Windows 7 and up
        COM_INTERFACE_ENTRY(IExplorerPaneVisibility) // used by Windows Vista and up.
    END_COM_MAP()


    CShellFolder()
    {
        // Register the columns the folder supports in 'detailed' mode.
        RegisterColumn(IDS_SHELLEXT_NAME, LVCFMT_LEFT);
        RegisterColumn(IDS_SHELLEXT_SIZE, LVCFMT_RIGHT);
    }


    // Purpose: called by MSF when the shellfolder needs to show a subfolder.
    void InitializeSubFolder(const CVVVItemList& items)
    {
        m_strSubFolder.Empty();

        for (CVVVItemList::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (!m_strSubFolder.IsEmpty())
            {
                m_strSubFolder += _T("\\");
            }

            m_strSubFolder += ToString(it->GetID());
        }
    }


    // Purpose: Create the shellfolderviewcb that will be used to catch callback events
    //          generated by the system folder view.
    CComPtr<IShellFolderViewCB> CreateShellFolderViewCB()
    {
        return CShellFolderViewCB::CreateInstance(GetRootFolder());
    }


    // Purpose: called by MSF/shell when a number of items are selected and a IDataObject
    //          that contains the items is required.
    CComPtr<IDataObject> CreateDataObject(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST* ppidl)
    {
        return CShellFolderDataObject::CreateInstance(pidlFolder, cidl, ppidl, this);
    }


    // Purpose: called by MSF/shell when it want the current list of 
    //          all items  The shell will walk all IDs and then release the enum.
    CComPtr<IEnumIDList> CreateEnumIDList(HWND /*hwnd*/, DWORD grfFlags)
    {
        return CEnumIDList::CreateInstance(GetPathFolderFile(), m_strSubFolder, grfFlags);
    }


    // Purpose: called by MSF when there is no global settings for all items.
    SFGAOF GetAttributeOf(unsigned int cidl, const CVVVItem& item, SFGAOF /*sfgofMask*/) const
    {
        return item.GetAttributeOf(cidl == 1, IsReadOnly(GetPathFolderFile()));
    }


    // Purpose: called by MSF to tell the shell which panes to show.
    // It is essential to override this function to control which explorer panes are visible as by default no panes are shown.
    EXPLORERPANESTATE GetPaneState(_In_ REFEXPLORERPANE ep)
    {
        if (ep == __uuidof(MSF::EP_Ribbon))
            return EPS_DEFAULT_ON;

        return EPS_DONTCARE;
    }


    // Purpose: called by the default context menu. Gives an option to merge
    //          extra commands into the menu.
    HRESULT OnDfmMergeContextMenu(IDataObject* pdataobject, UINT /*uFlags*/, QCMINFO& qcminfo)
    {
        CCfShellIdList itemlist(pdataobject);

        if (itemlist.GetItemCount() == 1 && !CVVVItem(itemlist.GetItem(0)).IsFolder())
        {
            // Add 'open' if only 1 item is selected.
            CMenu menu(true);
            menu.AddDefaultItem(ID_DFM_CMD_OPEN, _T("&Open"));
            MergeMenus(qcminfo, menu);

            // Note: XP will automatic make first menu item the default.
            //       Win98, ME and 2k don't do this, so must add as default item.
        }

        return S_OK;
    }


    // Purpose: Called to get the help string for added menu items.
    CString OnDfmGetHelpText(unsigned short nCmdId)
    {
        return LoadString(IDS_SHELLFOLDER_DFM_HELP_BASE + nCmdId);
    }


    HRESULT OnDfmInvokeAddedCommand(HWND hwnd, IDataObject* pdataobject, int nId)
    {
        switch (nId)
        {
            case ID_DFM_CMD_OPEN:
                OnOpen(hwnd, pdataobject);
                break;

            default:
                ATLASSERT(false); // unknown command id detected.
                break;
        }

        return S_OK;
    }


    // Purpose: handle 'open' by showing the name of the selected item.
    void OnOpen(HWND hwnd, IDataObject* pdataobject)
    {
        CCfShellIdList cfshellidlist(pdataobject); 
        ATLASSERT(cfshellidlist.GetItemCount() == 1);

        CVVVItem item(cfshellidlist.GetItem(0));

        if (item.IsFolder())
        {
            GetShellBrowser().BrowseObject(item.GetItemIdList(),
                SBSP_DEFBROWSER | SBSP_RELATIVE);
        }
        else
        {
            CString strMessage = _T("Open on: ") + item.GetName();
            IsolationAwareMessageBox(hwnd, strMessage, _T("Open"), MB_OK | MB_ICONQUESTION);
        }
    }


    // Purpose: Called by the shell/MSF when an item must be renamed.
    LPITEMIDLIST OnSetNameOf(HWND /*hwnd*/, const CVVVItem& item, const TCHAR* szNewName, SHGDNF shgndf)
    {
        RaiseExceptionIf(shgndf != SHGDN_NORMAL && shgndf != SHGDN_INFOLDER); // not supported 'name'.

        CPidl pidl(CVVVItem::CreateItemIdList(item.GetID(), item.GetSize(), item.IsFolder(), szNewName));

        CVVVFile(GetPathFolderFile(), m_strSubFolder).SetItem(CVVVItem(pidl));

        return pidl.Detach();
    }


    // Purpose: handles the 'properties request.
    //          The property sheet/page allows the user to change 
    //          the name and size of an item.
    long OnProperties(HWND hwnd, CVVVItemList& items)
    {
        ATLASSERT(items.size() == 1);
        CVVVItem& item = items[0];

        long wEventId;
        if (CVVVPropertySheet(item, this).DoModal(hwnd, wEventId) > 0 && wEventId != 0)
        {
            CVVVFile vvvfile(GetPathFolderFile(), m_strSubFolder);
            vvvfile.SetItem(item);
        }

        return wEventId;
    }


    // Purpose: Called by MSF/shell when items must be deleted.
    long OnDelete(HWND hwnd, CVVVItemList& items)
    {
        if (!hwnd && !UserConfirmsFileDelete(hwnd, items))
            return 0; // user wants to abort the file deletion process.

        CVVVFile(GetPathFolderFile(), m_strSubFolder).DeleteItems(items);

        return SHCNE_DELETE;
    }


    // Purpose: called by the standard MSF drag handler during drag operations.
    bool IsSupportedClipboardFormat(IDataObject* pdataobject)
    {
        return CCfHDrop::IsFormat(pdataobject);
    }


    // Purpose: called when items are pasted or droped on the shellfolder.
    DWORD AddItemsFromDataObject(DWORD dwEffect, IDataObject* pdataobject)
    {
        CCfHDrop cfhdrop(pdataobject);

        unsigned int nFiles = cfhdrop.GetFileCount();
        for (unsigned int i = 0; i < nFiles; ++i)
        {
            AddItem(cfhdrop.GetFile(i));
        }

        // The VVV sample cannot use optimized move. Just return dwEffect as passed.
        return dwEffect;
    }


    void OnError(HRESULT hr, HWND hwnd, EErrorContext /*errorcontext*/)
    {
        CString strMsg = LoadString(IDS_SHELLFOLDER_CANNOT_PERFORM) + FormatLastError(static_cast<DWORD>(hr));

        IsolationAwareMessageBox(hwnd, strMsg,
            LoadString(IDS_SHELLEXT_ERROR_CAPTION), MB_OK | MB_ICONERROR);
    }

private:

    // Purpose: Ask the user if he is really sure about the file delete action.
    //          Deleted files cannot be restored from the recycle bin.
    bool UserConfirmsFileDelete(HWND hwnd, const CVVVItemList& items)
    {
        CString strMessage;
        UINT    nCaptionResId;

        if (items.size() == 1)
        {
            strMessage.FormatMessage(IDS_SHELLFOLDER_DELETE, items[0].GetDisplayName().GetString());
            nCaptionResId = IDS_SHELLFOLDER_FILE_DELETE_CAPTION;
        }
        else
        {
            strMessage.FormatMessage(IDS_SHELLFOLDER_MULTIPLE_DELETE,
                ToString(static_cast<unsigned int>(items.size())).GetString());
            nCaptionResId = IDS_SHELLFOLDER_FILES_DELETE_CAPTION;
        }

        return IsolationAwareMessageBox(hwnd, strMessage,
            LoadString(nCaptionResId), MB_YESNO | MB_ICONQUESTION) == IDYES;
    }


    void AddItem(const CString& strFile)
    {
        CVVVFile vvvfile(GetPathFolderFile(), m_strSubFolder);

        CPidl pidlItem(vvvfile.AddItem(strFile));

        ReportAddItem(pidlItem);
    }


    bool IsReadOnly(const CString& strFileName) const
    {
        DWORD dwAttributes = GetFileAttributes(strFileName);
        return (dwAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    }


    // Member variables
    CString m_strSubFolder;
};


OBJECT_ENTRY_AUTO(__uuidof(CShellFolder), CShellFolder)
