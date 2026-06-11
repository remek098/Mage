using MageEditor.DllWrappers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Interop;

namespace MageEditor.Utilities
{
    // NOTE: HwndHost inherits from FrameworkElement, so we can use it in our UI
    class RenderSurfaceHost : HwndHost
    {
        private IntPtr _renderSurfaceWindowHandle = IntPtr.Zero;
        private readonly int _width = 800;
        private readonly int _height = 600;
        private DelayedEventTimer _resizeTimer;

        public int SurfaceId { get; private set; } = ID.INVALID_ID;


        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            SurfaceId = EngineAPI.CreateRenderSurface(hwndParent.Handle, _width, _height);
            Debug.Assert(ID.IsValid(SurfaceId));

            _renderSurfaceWindowHandle = EngineAPI.GetWindowHandle(SurfaceId);
            Debug.Assert(_renderSurfaceWindowHandle != IntPtr.Zero);

            return new HandleRef(this, _renderSurfaceWindowHandle);

        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            EngineAPI.RemoveRenderSurface(SurfaceId);
            SurfaceId = ID.INVALID_ID;
            _renderSurfaceWindowHandle = IntPtr.Zero;
        }


        public void Resize()
        {
            // NOTE: Resize() will be called multiple times, the resizing will be held off until for some time it's not
            // being called again.
            // Which is good, since we handle resizing through WM_SIZE message
            _resizeTimer.Trigger();
        }

        private void Resize(object? sender, DelayedEventTimerArgs e)
        {
            e.RepeatEvent = Mouse.LeftButton == MouseButtonState.Pressed;
            if(!e.RepeatEvent)
            {
                EngineAPI.ResizeRenderSurface(SurfaceId);

                Logger.Log(MessageType.Info, "Resized RenderSurfaceHost.");
            }
        }

        public RenderSurfaceHost(double width, double height)
        {
            _width = (int)width;
            _height = (int)height;

            // quarter of a second -> if for this amount of time Resize() was not called, it will call this function for us
            _resizeTimer = new DelayedEventTimer(TimeSpan.FromMilliseconds(250.0));
            _resizeTimer.Triggered += Resize;
        }

    }
}
