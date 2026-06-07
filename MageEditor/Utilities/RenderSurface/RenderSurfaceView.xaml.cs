using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MageEditor.Utilities
{
    /// <summary>
    /// Interaction logic for RenderSurfaceView.xaml
    /// </summary>
    public partial class RenderSurfaceView : UserControl, IDisposable
    {
        private RenderSurfaceHost? _host = null;

        public RenderSurfaceView()
        {
            InitializeComponent();
            Loaded += OnRenderSurfaceViewLoaded;
        }

        /// <summary>
        /// Doing it this way instead of doing it in constructor, because during constructor view might still not have ActualWidth or ActualHeight,
        /// <para>because object might still not be placed inside the visual tree for WPF.</para>
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnRenderSurfaceViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnRenderSurfaceViewLoaded;

            _host = new RenderSurfaceHost(ActualWidth, ActualHeight);
            Content = _host;
        }

        #region IDisposable support
        private bool _disposedValue;

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposedValue)
            {
                if (disposing)
                {
                    // RenderSurfaceHost implements HwndHost, which implements IDisposable interface
                    _host?.Dispose();
                }

                _disposedValue = true;
            }
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
        #endregion

    }
}
