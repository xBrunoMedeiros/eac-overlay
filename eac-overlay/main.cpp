#include <windows.h>
#include <iostream>
#include <d3d9types.h>
#include "features/overlay.hpp"


int main( )
{
    if ( overlay::initialize( ).was_successful( ) )
    {
        //
        HWND target = FindWindowA( NULL, "Among Us" );
        //
        overlay::main_loop( target, [ ]( bool is_focused, RECT& game_rect, ImDrawList* render )
        {
            // Ignore the callback when the game is not in focus.
            if ( !is_focused )
            {
                return Sleep( 100 );
            }

            // Rendering here
            for ( int i = 0; i < 1; i++ )
            {
                render->AddRect( { 0.0f, 0.0f }, { ( FLOAT ) ( game_rect.right - game_rect.left ), ( FLOAT ) ( game_rect.bottom - game_rect.top ) }, D3DCOLOR_RGBA( 0, 0, 255, 255 ) );
            }

            ImGui::Begin( "Framerate", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar );
            ImGui::SetWindowSize( ImVec2( 200, 30 ) );
            ImGui::SetWindowPos( ImVec2( 15, 80 ) );
            ImGui::Text( "%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO( ).Framerate, ImGui::GetIO( ).Framerate );
            ImGui::End( );
        } );
    }
    overlay::cleanup( );

    // std::cout << "Style: " << std::hex << GetWindowLongPtrW( overlay::window_handle, GWL_STYLE ) << std::endl;
    // std::cout << "ExStyle: " << std::hex << GetWindowLongPtrW( overlay::window_handle, GWL_EXSTYLE ) << std::endl;
}