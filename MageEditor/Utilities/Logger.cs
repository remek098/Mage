using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Interop;
using System.Windows.Shapes;
using Path = System.IO.Path;

namespace MageEditor.Utilities
{
    enum MessageType
    {
        Info = 0x01,
        Warning = 0x02,
        Error = 0x04,

    }

    class LogMessage
    {
        public DateTime Time { get; }

        public MessageType MessageType { get; }

        public string Message { get; }
        public string Caller { get; }
        public string File { get; }
        public int Line { get; }

        public string MetaData => $"{File}: {Caller} ({Line})";

        public LogMessage(MessageType type, string msg, string file, string caller, int line)
        {
            Time = DateTime.Now;
            MessageType = type;
            Message = msg;
            File = Path.GetFileName(file);
            Caller = caller;
            Line = line;
        }
    }

    static class Logger
    {

        private static int _messageFilter = (int)(MessageType.Info | MessageType.Warning | MessageType.Error);
        private static readonly ObservableCollection<LogMessage> _messages = [];

        public static ReadOnlyObservableCollection<LogMessage> Messages { get; }
            = new ReadOnlyObservableCollection<LogMessage>(_messages);

        public static CollectionViewSource FilteredMessages { get; }
            = new CollectionViewSource() { Source = Messages };


        // this function in future will be called from possibly other thread than UI thread.
        public static async void Log(MessageType type, string msg,
            [CallerFilePath] string file = "", [CallerMemberName] string caller = "",
            [CallerLineNumber] int line = 0)
        {
            // using current dispatcher of this application (kinda like main thread for this application)
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Add(new LogMessage(type, msg, file, caller, line));
            }));
        }

        public static async void Clear()
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Clear();
            }));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="mask">mask is the bit flag that is desiphered with MessageType enum</param>
        public static void SetMessageFilter(int mask)
        {
            _messageFilter = mask;
            FilteredMessages.View.Refresh();
        }

        
        static Logger()
        {
            // lambda expression that filters the FilteredMessages.View every single  time
            // FilteredMessages.View.Refresh() is called.
            // basically we are trying to see whether or not our message should be in messages list
            // based on our prefered flags/filter
            FilteredMessages.Filter += (s, e) =>
            {
                if (e.Item != null)
                {
                    // e.g. if _messageFilters == 0101 -> it means we display Info, Errors
                    int type = (int)((LogMessage)e.Item).MessageType;
                    e.Accepted = (type & _messageFilter) != 0;
                }
            };
        }
        
    }
}
