using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
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
        private enum Win32Message
        {
            WM_SIZING         = 0x0214,
            WM_ENTERSIZEMOVE  = 0x0231,
            WM_EXITSIZEMOVE   = 0x0232,
            WM_SIZE           = 0x0005,
        }


        private RenderSurfaceHost? _host = null;
        // private bool _canResize = true;
        // private bool _isBeingMoved = false;


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
            _host.MessageHook += new HwndSourceHook(HostMsgFilter);
            Content = _host;
            
            //// now we can find any particular window (not just MainWindow) that has RenderSurfaceView and attach resizing HwndMessageHook
            //var window = this.FindVisualParent<Window>();
            //Debug.Assert(window != null);

            //var helper = new WindowInteropHelper(window);
            //if(helper != null)
            //{
            //    // now resize messages will also be sent and received and handled inside HwndMessageHook
            //    // mainly for when changing size of MainWindow
            //    HwndSource.FromHwnd(helper.Handle)?.AddHook(HwndMessageHook);
            //}
        }


        //private nint HwndMessageHook(nint hwnd, int msg, nint wParam, nint lParam, ref bool handled)
        //{
        //    switch ((Win32Message)msg)
        //    {
        //        // NOTE: handling resizing tthere because internal_wnd_proc in engine doesn't handle resizing.
        //        // 1) we want to keep client_area resized properly
        //        // 2) render_surface has to be kept updated on resize
        //        case Win32Message.WM_SIZING:
        //            _canResize = false;
        //            _isBeingMoved = false;
        //            break;
        //        case Win32Message.WM_ENTERSIZEMOVE:
        //            _isBeingMoved = true;
        //            break;
        //        case Win32Message.WM_EXITSIZEMOVE:
        //            _canResize = true;
        //            if (!_isBeingMoved)
        //            {
        //                _host?.Resize();
        //            }
        //            break;

        //        default:
        //            break;
        //    }

        //    return IntPtr.Zero; // returning 0, because we don't do anything special.
        //}

        // NOTE: very similar to wnd_proc -> handled indicates whether we want to let window know if we handled event or not
        private nint HostMsgFilter(nint hwnd, int msg, nint wParam, nint lParam, ref bool handled)
        {
            // NOTE: no border for hosted window, therefore this window cannot know if it entered any resize,
            // knows only it's size has been changed
            switch((Win32Message)msg)
            {
                // NOTE: handling resizing tthere because internal_wnd_proc in engine doesn't handle resizing.
                // 1) we want to keep client_area resized properly
                // 2) render_surface has to be kept updated on resize
                case Win32Message.WM_SIZING: throw new Exception();
                case Win32Message.WM_ENTERSIZEMOVE: throw new Exception();
                case Win32Message.WM_EXITSIZEMOVE: throw new Exception();
                case Win32Message.WM_SIZE:
                    //if(_canResize)
                    //{
                    //    _host?.Resize();
                    //}
                    break;
                default:
                    break;
            }

            return IntPtr.Zero; // returning 0, because we don't do anything special.
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
