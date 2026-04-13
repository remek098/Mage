using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MageEditor.Dictionaries
{
    public partial class ControlTemplates : ResourceDictionary
    {
        private void OnTextBoxRename_KeyDown(object sender, KeyEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox?.GetBindingExpression(TextBox.TextProperty);
            if (e.Key == Key.Enter)
            {
                if (textBox?.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);
                }
                else
                {
                    exp?.UpdateSource();
                }
                if(textBox != null) textBox.Visibility = Visibility.Collapsed;
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                exp?.UpdateTarget(); // means we read the old value and then clear focus so that we don't type in that TextBox anymore
                if (textBox != null) textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnTextBoxRename_LostFocus(object sender, RoutedEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox?.GetBindingExpression(TextBox.TextProperty);
            if (exp != null && textBox is not null)
            {
                exp.UpdateTarget();
                textBox.MoveFocus(new TraversalRequest(FocusNavigationDirection.Previous));
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnTextBox_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox?.GetBindingExpression(TextBox.TextProperty);
            if (e.Key == Key.Enter)
            {
                if(textBox?.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);
                }
                else
                {
                    exp?.UpdateSource();
                }
                Keyboard.ClearFocus();
                e.Handled = true;
            }
            else if(e.Key == Key.Escape)
            {
                exp?.UpdateTarget(); // means we read the old value and then clear focus so that we don't type in that TextBox anymore
                Keyboard.ClearFocus();
            }
        }

        private void OnClose_Button_Click(object sender, RoutedEventArgs e)
        {
            // that should be a window, because we're using it inside a template -> templated parent of that button should be our window
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.Close();
        }

        private void OnMaximizeRestore_Button_Click(object sender, RoutedEventArgs e)
        {
            // that should be a window, because we're using it inside a template -> templated parent of that button should be our window
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState =  window.WindowState == WindowState.Normal ? 
                WindowState.Maximized : WindowState.Normal;
        }

        private void OnManimize_Button_Click(object sender, RoutedEventArgs e)
        {
            // that should be a window, because we're using it inside a template -> templated parent of that button should be our window
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState = WindowState.Minimized;
        }

        
    }
}
