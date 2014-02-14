/*
 * Copyright 2014 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wmp_private.h"
#include "olectl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmp);

struct WindowsMediaPlayer {
    IOleObject IOleObject_iface;
    IProvideClassInfo2 IProvideClassInfo2_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;

    LONG ref;

    IOleClientSite *client_site;
};

static void release_client_site(WindowsMediaPlayer *This)
{
    if(!This->client_site)
        return;

    IOleClientSite_Release(This->client_site);
    This->client_site = NULL;
}

static inline WindowsMediaPlayer *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IOleObject_iface);
}

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(riid, &IID_IOleObject)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(riid, &IID_IProvideClassInfo)) {
        TRACE("(%p)->(IID_IProvideClassInfo %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(riid, &IID_IProvideClassInfo2)) {
        TRACE("(%p)->(IID_IProvideClassInfo2 %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(riid, &IID_IPersist)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(riid, &IID_IPersistStreamInit)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(riid, &IID_IOleWindow)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IOleInPlaceObject)) {
        TRACE("(%p)->(IID_IOleInPlaceObject %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IOleInPlaceObjectWindowless)) {
        TRACE("(%p)->(IID_IOleInPlaceObjectWindowless %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(riid, &IID_IConnectionPointContainer)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &This->IConnectionPointContainer_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_client_site(This);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    IOleControlSite *control_site;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    release_client_site(This);
    if(!pClientSite)
        return S_OK;

    IOleClientSite_AddRef(pClientSite);
    This->client_site = pClientSite;

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleControlSite, (void**)&control_site);
    if(SUCCEEDED(hres)) {
        IDispatch *disp;

        hres = IOleControlSite_GetExtendedControl(control_site, &disp);
        if(SUCCEEDED(hres) && disp) {
            FIXME("Use extended control\n");
            IDispatch_Release(disp);
        }

        IOleControlSite_Release(control_site);
    }

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    *ppClientSite = This->client_site;
    return This->client_site ? S_OK : E_FAIL;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%08x)\n", This, dwSaveOption);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p %d %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                        DWORD dwReserved)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %d)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p %p %d %p %p)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%d %p)\n", This, dwAspect, pdwStatus);

    switch(dwAspect) {
    case DVASPECT_CONTENT:
        *pdwStatus = OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
            |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE;
        break;
    default:
        FIXME("Unhandled aspect %d\n", dwAspect);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    WindowsMediaPlayer *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static inline WindowsMediaPlayer *impl_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI OleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI OleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface, HWND *phwnd)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p %p)\n", This, lprcPosRect, lprcClipRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %lu %lu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    WindowsMediaPlayer *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObjectWindowless_QueryInterface,
    OleInPlaceObjectWindowless_AddRef,
    OleInPlaceObjectWindowless_Release,
    OleInPlaceObjectWindowless_GetWindow,
    OleInPlaceObjectWindowless_ContextSensitiveHelp,
    OleInPlaceObjectWindowless_InPlaceDeactivate,
    OleInPlaceObjectWindowless_UIDeactivate,
    OleInPlaceObjectWindowless_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

static inline WindowsMediaPlayer *impl_from_IProvideClassInfo2(IProvideClassInfo2 *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IProvideClassInfo2_iface);
}

static HRESULT WINAPI ProvideClassInfo2_QueryInterface(IProvideClassInfo2 *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI ProvideClassInfo2_AddRef(IProvideClassInfo2 *iface)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI ProvideClassInfo2_Release(IProvideClassInfo2 *iface)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI ProvideClassInfo2_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ppTI)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);
    FIXME("(%p)->(%p)\n", This, ppTI);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideClassInfo2 *iface, DWORD dwGuidKind, GUID *pGUID)
{
    WindowsMediaPlayer *This = impl_from_IProvideClassInfo2(iface);

    TRACE("(%p)->(%d %p)\n", This, dwGuidKind, pGUID);

    if(dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID) {
        FIXME("Unexpected dwGuidKind %d\n", dwGuidKind);
        return E_INVALIDARG;
    }

    *pGUID = IID__WMPOCXEvents;
    return S_OK;
}

static const IProvideClassInfo2Vtbl ProvideClassInfo2Vtbl = {
    ProvideClassInfo2_QueryInterface,
    ProvideClassInfo2_AddRef,
    ProvideClassInfo2_Release,
    ProvideClassInfo2_GetClassInfo,
    ProvideClassInfo2_GetGUID
};

static inline WindowsMediaPlayer *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IPersistStreamInit_iface);
}

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                       REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, LPSTREAM pStm)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pStm);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, LPSTREAM pStm,
                                             BOOL fClearDirty)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p %x)\n", This, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                   ULARGE_INTEGER *pcbSize)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    WindowsMediaPlayer *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

static inline WindowsMediaPlayer *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID riid, LPVOID *ppv)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppCP);
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

HRESULT WINAPI WMPFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    WindowsMediaPlayer *wmp;
    HRESULT hres;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    wmp = heap_alloc_zero(sizeof(*wmp));
    if(!wmp)
        return E_OUTOFMEMORY;

    wmp->IOleObject_iface.lpVtbl = &OleObjectVtbl;
    wmp->IProvideClassInfo2_iface.lpVtbl = &ProvideClassInfo2Vtbl;
    wmp->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    wmp->IOleInPlaceObjectWindowless_iface.lpVtbl = &OleInPlaceObjectWindowlessVtbl;
    wmp->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;

    wmp->ref = 1;

    hres = IOleObject_QueryInterface(&wmp->IOleObject_iface, riid, ppv);
    IOleObject_Release(&wmp->IOleObject_iface);
    return hres;
}