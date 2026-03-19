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
    }
}
