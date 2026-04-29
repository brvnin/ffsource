#pragma once
#include <d3d11.h>

namespace DiscordAvatar
{
    // Download e carrega o avatar da URL, retorna a ShaderResourceView
    ID3D11ShaderResourceView* LoadFromUrl(const char* url, ID3D11Device* device);

    // Libera a ShaderResourceView
    void FreeTexture(ID3D11ShaderResourceView* texture);
}