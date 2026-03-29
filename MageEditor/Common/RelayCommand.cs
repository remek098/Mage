using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MageEditor.Common
{
    class RelayCommand<T> : ICommand
    {
        private readonly Action<T> _execute;
        private readonly Predicate<T>? _canExecute;

        public event EventHandler? CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        public bool CanExecute(object? parameter)
        {
            return parameter is T value
                ? _canExecute?.Invoke(value) ?? true
                : true;
        }

        public void Execute(object? parameter)
        {
            T value = parameter is T v ? v : default!;
            _execute(value);

            //_execute((T)parameter);

            //if (parameter is T value)
            //{
            //    _execute((T)parameter);
            //}
            //else
            //{
            //    Debug.WriteLine("Tried to call RelayCommand<T>.Execute() with null parameter. Not allowed.");
            //}
        }

        public RelayCommand(Action<T> execute, Predicate<T>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }
    }
}
