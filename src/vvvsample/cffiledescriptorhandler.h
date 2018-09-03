﻿//
// (C) Copyright by Victor Derks
//
// See README.TXT for the details of the software license.
//
#pragma once

#include "vvvitem.h"
#include <msf.h>
#include <strsafe.h>

// Note: owner of the instance of this class must keep passed data object alive.
//       This class doesn't do increases the reference count to prevent circular referencing.

class CCfFileDescriptorHandler : public msf::ClipboardFormatHandler
{
public:
    explicit CCfFileDescriptorHandler(IDataObject* dataObject) :
        ClipboardFormatHandler(CFSTR_FILEDESCRIPTOR, true, false),
        m_dataObject(dataObject)
    {
    }

    ~CCfFileDescriptorHandler() = default;
    CCfFileDescriptorHandler(const CCfFileDescriptorHandler&) = delete;
    CCfFileDescriptorHandler(CCfFileDescriptorHandler&&) = delete;
    CCfFileDescriptorHandler& operator=(const CCfFileDescriptorHandler&) = delete;
    CCfFileDescriptorHandler& operator=(CCfFileDescriptorHandler&&) = delete;

    void GetData(const FORMATETC&, STGMEDIUM& medium) const override
    {
        msf::CCfShellIdList cfshellidlist(m_dataObject);

        // Note: FILEGROUPDESCRIPTOR provides the count and 1 FILEDESCRIPTOR.
        const size_t size = sizeof(FILEGROUPDESCRIPTOR) + ((cfshellidlist.GetItemCount() - 1) * sizeof(FILEDESCRIPTOR)); // -V119

        const HGLOBAL hg = msf::GlobalAllocThrow(size);
        auto* fileGroupDescriptor = static_cast<FILEGROUPDESCRIPTOR*>(hg);

        fileGroupDescriptor->cItems = static_cast<UINT>(cfshellidlist.GetItemCount());
        for (unsigned int i = 0; i < fileGroupDescriptor->cItems; ++i)
        {
            FILEDESCRIPTOR& fd = fileGroupDescriptor->fgd[i];

            fd.dwFlags = FD_FILESIZE;
            fd.nFileSizeHigh = 0;

            const VVVItem vvvItem(cfshellidlist.GetItem(i));

            fd.nFileSizeLow = std::min(vvvItem.GetSize(), MAX_VVV_ITEM_SIZE);
            msf::RaiseExceptionIfFailed(StringCchCopy(fd.cFileName, MAX_PATH, vvvItem.GetDisplayName().c_str()));
        }

        msf::StorageMedium::SetHGlobal(medium, hg);
    }

private:
    IDataObject* m_dataObject;
};
