using MageEditor.Content;

namespace MageEditor.Editors
{
    interface IAssetEditor
    {
        Asset Asset { get; }

        void SetAsset(AssetInfo asset);
    }
}