using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace MageEditor.Utilities
{
    public static class ID
    {
        // this stuff has to be changed if in Engine project in Common/Id.h
        // the id_type would be changed to smth then u32
        public static int INVALID_ID => -1;

        public static bool IsValid(int id) => id != INVALID_ID;
    }

    public static class MathUtil
    {
        public static float Epsilon => 0.00001f;

        // extension method
        public static bool IsTheSameAs(this float value, float other)
        {
            return Math.Abs(value - other) < Epsilon;
        }

        public static bool IsTheSameAs(this float? value, float? other)
        {
            if(!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }

        public static long AlignSizeUp(long size, long alignment)
        {
            /*
            nice and fast way of checking if a single bit is set: value && !(value & (value - 1)):
            value:      0010 0000
            value-1:    0001 1111
            This gives  !(0010 0000 & 0001 1111) => 1111 1111
            i.e. if any of bits after most significant set bit is set, therefore value & (value -1) would be non-zero
            */
            Debug.Assert(alignment > 0, "Alignment must be non-zero."); // non-zero alignment is required.
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment should be a power of 2."); // the !(value & (value -1)) part


            // add a mask and clean up mask bits.
            return ((size + mask) & ~mask);
        }

        // align by rounding down. Will result in a multiple of "alignment" that is less than or equal to 'size'
        public static long AlignSizeDown(long size, long alignment)
        {
            Debug.Assert(alignment > 0, "Alignment must be non-zero."); // non-zero alignment is required.
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment should be a power of 2.");
            // clean up mask bits -> i.e. align to most significant bit's power of 2.
            return (size & ~mask);
        }

    }

    
    class DelayedEventTimerArgs : EventArgs
    {
        public bool RepeatEvent { get; set; }
        public IEnumerable<object> Data { get; set; }

        public DelayedEventTimerArgs(IEnumerable<object> data)
        {
            Data = data;
        }
    }

    class DelayedEventTimer
    {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private readonly List<object> _data = new List<object>();
        private DateTime _lastEventTime = DateTime.Now;

        // DelayedEvent might call an event again after _delay time
        public event EventHandler<DelayedEventTimerArgs> Triggered;

        public void Trigger(object? data = null)
        {
            if(data != null) {
                _data.Add(data);
            }
            _lastEventTime = DateTime.Now;
            _timer.IsEnabled = true;
        }

        public void Disable()
        {
            _timer.IsEnabled = false;
        }
        private void OnTimerTick(object? sender, EventArgs e)
        {
            if ((DateTime.Now - _lastEventTime) < _delay) return;
            var eventArgs = new DelayedEventTimerArgs(_data);
            Triggered?.Invoke(this, eventArgs);
            if(!eventArgs.RepeatEvent) {
                _data.Clear();
            }
            _timer.IsEnabled = eventArgs.RepeatEvent;
        }

        public DelayedEventTimer(TimeSpan delay, DispatcherPriority priority = DispatcherPriority.Normal)
        {
            _delay = delay;
            _timer = new DispatcherTimer(priority)
            {
                Interval = TimeSpan.FromMilliseconds(delay.TotalMilliseconds * 0.5)
            };
            _timer.Tick += OnTimerTick;
        }

    }
}
