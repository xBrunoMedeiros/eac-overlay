#pragma once
#include <windows.h>
#include <functional>
#include <d3d11_2.h>
#include <wrl.h>
#include <expected.hpp>
#include <imgui/imgui.h>

/*
	Bypassing - Easy Anti Cheat

	In addition to this configuration, is required the WindowRect exactly the same as the game,
	and the window is only checked when it has WS_VISIBLE defined.
*/
#define EAC_WINDOW_TITLE	L"BiGlH"
#define EAC_WINDOW_CLASS	L"BiGlH"
#define EAC_WINDOW_STYLE	WS_POPUP | WS_CLIPSIBLINGS /*| WS_VISIBLE*/
#define EAC_WINDOW_EXSTYLE	WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST

namespace overlay
{
	// Window data
	extern HWND window_handle;

	// DirectX11 data
	extern Microsoft::WRL::ComPtr<ID3D11Device> device;
	extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
	extern Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	extern Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;

	// 
	typedef std::function<void( bool is_focused, RECT& target_rect, ImDrawList* renderer )> t_render_callback;

	//
	util::Expected<void> initialize( );
	void cleanup( );
	void main_loop( HWND target_window, t_render_callback render_callback );
}