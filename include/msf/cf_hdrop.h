﻿//
// (C) Copyright by Victor Derks
//
// See README.TXT for the details of the software licence.
//
#pragma once

#include "smartptr/dataobjectptr.h"
#include "stg_medium.h"
#include "format_etc.h"

namespace msf
{

/// <summary>Support class to handle the CF_HDROP format.</summary>
/// <remarks>
/// The CF_HDROP format is used by the shell to transfer a group of existing files.
/// The handle refers to a set of DROPFILES structures.
/// </remarks>
class ClipboardFormatHDrop
{
public:
    /// <summary>Returns true when the data object has the CF_HDROP format.</summary>
    static bool IsFormat(_In_ IDataObject* dataObject)
    {
        FormatEtc formatEtc(CF_HDROP);
        return SUCCEEDED(dataObject->QueryGetData(&formatEtc));
    }

    explicit ClipboardFormatHDrop(IDataObjectPtr dataobject)
    {
        dataobject.GetData(FormatEtc(CF_HDROP), m_stgmedium);
    }

    [[nodiscard]] bool IsEmpty() const noexcept
    {
        return GetFileCount() == 0;
    }

    [[nodiscard]] uint32_t GetFileCount() const noexcept
    {
        ATLASSERT(m_stgmedium.tymed == TYMED_HGLOBAL && "Unable to retrieve filecount");
        return ::DragQueryFile(static_cast<HDROP>(m_stgmedium.hGlobal), static_cast<uint32_t>(-1), nullptr, 0);
    }

    [[nodiscard]] std::wstring GetFile(unsigned int index) const
    {
        ATLASSERT(index < GetFileCount() && "Index out of bounds");

        wchar_t szFileName[MAX_PATH];
        if (!::DragQueryFile(static_cast<HDROP>(m_stgmedium.hGlobal), index, szFileName,
                             _countof(szFileName)))
            throw _com_error(HRESULT_FROM_WIN32(GetLastError()));

        return szFileName;
    }

private:
    StorageMedium m_stgmedium;
};

} // namespace msf
