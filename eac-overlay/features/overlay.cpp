#include "overlay.hpp"
#include <iostream>
#include <d2d1_2.h>
#include <dcomp.h>
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// Window data
HWND overlay::window_handle = NULL;

// DirectX data
Microsoft::WRL::ComPtr<ID3D11Device> overlay::device = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> overlay::device_context = nullptr;
Microsoft::WRL::ComPtr<IDXGISwapChain1>	overlay::swap_chain = nullptr;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> overlay::render_target_view = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

util::Expected<void> overlay::initialize( )
{
    WNDCLASS window_class = { };
    window_class.lpszClassName = EAC_WINDOW_CLASS;
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = [ ]( HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param ) -> LRESULT WINAPI
    {
        if ( ImGui_ImplWin32_WndProcHandler( window_handle, msg, w_param, l_param) )
        {
            return 1;
        }
        return DefWindowProc( window_handle, msg, w_param, l_param );
    };

    // Register the class for the overlay
    if ( FAILED( RegisterClass( &window_class ) ) )
    {
        return std::exception( "There was an error registering the class for the overlay." );
    }

    // Create the window for the overlay
    window_handle = CreateWindowEx( EAC_WINDOW_EXSTYLE, EAC_WINDOW_CLASS, EAC_WINDOW_TITLE, EAC_WINDOW_STYLE, 0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr );
    if ( !window_handle )
    {
        return std::exception( "There was an error creating the window for the overlay." );
    }

    if ( FAILED( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                                    D3D11_SDK_VERSION, device.GetAddressOf( ), nullptr, device_context.GetAddressOf( ) ) ) )
    {
        return std::exception( "The creation of the device failed!" );
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    if ( FAILED( device.As( &dxgi_device ) ) )
    {
        return std::exception( "The Direct3D device was unable to retrieve the underlying DXGI device!" );
    }

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxFactory;
    if ( FAILED( CreateDXGIFactory2( DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS( dxFactory.GetAddressOf( ) ) ) ) )
    {
        return std::exception( "The creation of the factory failed!" );
    }

    DXGI_SWAP_CHAIN_DESC1 description;
    RtlZeroMemory( &description, sizeof( DXGI_SWAP_CHAIN_DESC1 ) );
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.Width = 1;                                              // Minimum size only for resize.
    description.Height = 1;
    description.SampleDesc.Count = 1;                                   // Don't use multi-sampling.
    description.SampleDesc.Quality = 0;
    description.BufferCount = 2;                                        // Use two buffers to enable flip effect.
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;          // flip mode and discared buffer after presentation.

    //
    if ( FAILED( dxFactory->CreateSwapChainForComposition( dxgi_device.Get( ), &description, nullptr, swap_chain.GetAddressOf( ) ) ) )
    {
        return std::exception( "The creation of the swap chain failed!" );
    }

    // 
    Microsoft::WRL::ComPtr<IDCompositionDevice> dcomp_device;
    if ( FAILED( DCompositionCreateDevice( dxgi_device.Get( ), IID_PPV_ARGS( dcomp_device.GetAddressOf( ) ) ) ) )
    {
        return std::exception( "The creation of the dc composition failed!" );
    }

    IDCompositionTarget* target;
    auto hr = dcomp_device->CreateTargetForHwnd( window_handle, true, &target );

    Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
    hr = dcomp_device->CreateVisual( visual.GetAddressOf( ) );

    hr = visual->SetContent( swap_chain.Get( ) );
    hr = target->SetRoot( visual.Get( ) );

    hr = dcomp_device->Commit( );


    // Setup ImGui context
    ImGui::CreateContext( );
    ImGui::StyleColorsClassic( );
    ImGui_ImplWin32_Init( window_handle );
    ImGui_ImplDX11_Init( device.Get( ), device_context.Get( ) );

    // Return successfully
    return { };
}

util::Expected<void> create_target_view( )
{
    // Smart pointer to back buffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
  
    // Get swap chain back buffer
    if ( FAILED( overlay::swap_chain->GetBuffer( 0, IID_PPV_ARGS( back_buffer.GetAddressOf( ) ) ) ) )
    {
        return std::exception( "There was an error getting the backbuffer." );
    }

    // Creates a render-target view for accessing resource data.
    if ( FAILED( overlay::device->CreateRenderTargetView( back_buffer.Get( ), NULL, &overlay::render_target_view ) ) )
    {
        return std::exception( "There was an error creating the render target view." );
    }

    // Return successfully operation
    return { };
}

void cleanup_target_view( )
{
    overlay::render_target_view.Reset( );
}

void overlay::cleanup( )
{
    cleanup_target_view( );
    swap_chain.Reset( );
    device_context.Reset( );
    device.Reset( );
}

void overlay::main_loop( HWND target_window, t_render_callback render_callback )
{
    //
    MSG msg{ };
    RtlZeroMemory( &msg, sizeof( MSG ) );

    while ( msg.message != WM_QUIT )
    {
        // Process messages
        if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != WM_NULL )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            continue;
        }
        //
        RECT overlay_rect;
        bool overlay_visible = IsWindowVisible( overlay::window_handle );
        bool overlay_running = GetWindowRect( overlay::window_handle, &overlay_rect );
        // 
        RECT target_rect;
        bool target_running = GetWindowRect( target_window, &target_rect );
        bool target_focused = target_window == GetForegroundWindow( );

        if ( !target_focused )
        {
            // Hide overlay if the game is not in the foreground
            if ( overlay_visible )
            {
                ShowWindow( overlay::window_handle, SW_HIDE );
            }
            // Close overlay if the target is not available.
            if ( !target_running )
            {
                break;
            }
            render_callback( false, target_rect, nullptr );
            continue;
        }
        std::cout << "Style: " << std::hex << GetWindowLongPtrW( overlay::window_handle, GWL_STYLE ) << std::endl;
        std::cout << "ExStyle: " << std::hex << GetWindowLongPtrW( overlay::window_handle, GWL_EXSTYLE ) << std::endl;

        // Show overlay if the game is in the foreground
        if ( !overlay_visible || overlay_rect != target_rect )
        {
            //
            SetWindowPos( overlay::window_handle, HWND_TOPMOST, target_rect.left, target_rect.top, target_rect.right - target_rect.left, target_rect.bottom - target_rect.top, SWP_SHOWWINDOW );
            //
            cleanup_target_view( );
            overlay::swap_chain->ResizeBuffers( 0, target_rect.right - target_rect.left, target_rect.bottom - target_rect.top, DXGI_FORMAT_UNKNOWN, 0 );
            create_target_view( );
        }
        
        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame( );
        ImGui_ImplWin32_NewFrame( );
        ImGui::NewFrame( );
            
        render_callback( true, target_rect, ImGui::GetOverlayDrawList( ) );

        ImGui::Render( );

        // 
        device_context->OMSetRenderTargets( 1, render_target_view.GetAddressOf( ), NULL );

        //
        ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

        //
        swap_chain->Present( 1, 0 );
    }
    cleanup_target_view( );
}