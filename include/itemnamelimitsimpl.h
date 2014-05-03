//
// (C) Copyright by Victor Derks
//
// See README.TXT for the details of the software licence.
//
#pragma once


#include <shobjidl.h>
#include "olestring.h"


namespace MSF
{

class IItemNameLimitsImplDefault
{
public:
    // Purpose: Override this function to control which chars are valid.
    static const TCHAR* GetValidChars()
    {
        return nullptr;
    }


    // Purpose: Override this function to control which chars are invalid.
    static const TCHAR* GetInvalidChars()
    {
        return nullptr;
    }
};

template <typename T, typename TItem,
          typename TItemV = IItemNameLimitsImplDefault, typename TItemI = IItemNameLimitsImplDefault>
class ATL_NO_VTABLE IItemNameLimitsImpl :
    public IItemNameLimits
{
public:
    // IItemNameLimits
    STDMETHOD(GetValidCharacters)(LPWSTR* ppwszValidChars, LPWSTR* ppwszInvalidChars)
    {
        try
        {
            ATLTRACE2(atlTraceCOM, 0, _T("IItemNameLimitsImpl::GetValidCharacters\n"));

            const TCHAR* pszValidChars = TItemV::GetValidChars();
            if (!pszValidChars)
            {
                *ppwszValidChars = nullptr;

                const TCHAR* pszInvalidChars = TItemI::GetInvalidChars();
                if (!pszInvalidChars)
                {
                    *ppwszInvalidChars = nullptr;
                }
                else
                {
                    *ppwszInvalidChars = COleString::Dup(pszInvalidChars);
                }
            }
            else
            {
                *ppwszValidChars   = COleString::Dup(pszValidChars);
                *ppwszInvalidChars = nullptr;
            }
            return S_OK;
        }
        MSF_COM_CATCH_HANDLER()
    }


    // Purpose: called by the shell to retrieve the max length.
    //          Lenght can depend on the current string. (UNC pathname has
    //          different length then local path, etc).
    //
    STDMETHOD(GetMaxLength)(__RPC__in_string LPCWSTR pszName, __RPC__out int* piMaxNameLen)
    {
        try
        {
            ATLTRACE2(atlTraceCOM, 0, _T("IItemNameLimitsImpl::GetMaxLength, name=%s\n"), pszName);

            // The TItem must implement the static function GetMaxItemNameLength.
            *piMaxNameLen = TItem::GetMaxNameLength(pszName);

            ATLASSERT(*piMaxNameLen >= 0 && "negative length?");
            return S_OK;
        }
        MSF_COM_CATCH_HANDLER()
    }
 };

} // namespace MSF
