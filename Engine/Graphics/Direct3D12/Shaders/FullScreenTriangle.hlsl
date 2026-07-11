struct VS_Output
{
    noperspective float4 Position : SV_Position;
    noperspective float2 UV : TEXCOORD;
};

VS_Output FullScreenTriangleVS(in uint VertexIdx : SV_VertexID)
{
    VS_Output output;
    
    // TODO: write fullscreen triangle code.
    output.Position = float4(0, 0, 0, 1);
    return output;
}