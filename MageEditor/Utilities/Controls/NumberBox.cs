using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MageEditor.Utilities.Controls
{
    // telling WPF to expect these 2
    [TemplatePart(Name ="PART_textBlock",  Type = typeof(TextBlock))]
    [TemplatePart(Name = "PART_textBox", Type = typeof(TextBox))]
    class NumberBox : Control
    {
        // storing parsed from string Value
        private double _originalValue;
        private double _mouseX_dragStart; // dragging only horizontally
        private bool _mouseCaptured = false;
        private bool _valueChanged = false;
        private double _sensitivity = 0.01;

        public double SensitivityMultiplier
        {
            get => (double)GetValue(SensitivityProperty);
            set => SetValue(SensitivityProperty, value);
        }

        // e.g. Text property in TextBox is a DependencyProperty
        public static readonly DependencyProperty SensitivityProperty = DependencyProperty.Register(
            nameof(SensitivityMultiplier),
            typeof(double),
            typeof(NumberBox),
            new PropertyMetadata(1.0));

        public string Value 
        {
            get => (string)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        // e.g. Text property in TextBox is a DependencyProperty
        public static readonly DependencyProperty ValueProperty = DependencyProperty.Register(
            nameof(Value), 
            typeof(string), 
            typeof(NumberBox),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            // override that method, to add our own behaviour for textBlock, e.g. mouse dragging to change value
            if(GetTemplateChild("PART_textBlock") is TextBlock textBlock)
            {
                textBlock.MouseLeftButtonDown += OnTextBlock_LMB_Down;
                textBlock.MouseLeftButtonUp += OnTextBlock_LMB_Up;
                textBlock.MouseMove += OnTextBlock_Mouse_Move;
            }
        }

        private void OnTextBlock_Mouse_Move(object sender, MouseEventArgs e)
        {
            // dragging with mouse and setting a value
            if(_mouseCaptured)
            {
                var mouseX = e.GetPosition(this).X;
                var diff = mouseX - _mouseX_dragStart;
                if (Math.Abs(diff) > SystemParameters.MinimumHorizontalDragDistance)
                {
                    if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control)) _sensitivity = 0.001;
                    else if (Keyboard.Modifiers.HasFlag(ModifierKeys.Shift)) _sensitivity = 0.1;
                    else _sensitivity = 0.01;
                    
                    var newValue = _originalValue + (diff * _sensitivity * SensitivityMultiplier);
                    Value = newValue.ToString("0.#####");
                    _valueChanged = true;
                }
            }
        }

        private void OnTextBlock_LMB_Up(object sender, MouseButtonEventArgs e)
        {
            if(_mouseCaptured)
            {
                Mouse.Capture(null);
                _mouseCaptured = false;
                e.Handled = true;

                // we want to type the value technically speaking
                if(!_valueChanged && GetTemplateChild("PART_textBox") is TextBox textBox)
                {
                    textBox.Visibility = Visibility.Visible;
                    textBox.Focus();
                    textBox.SelectAll();
                }
            }
        }

        private void OnTextBlock_LMB_Down(object sender, MouseButtonEventArgs e)
        {
            double.TryParse(Value, out _originalValue);
            Mouse.Capture(sender as UIElement);
            _mouseCaptured = true;
            _valueChanged = false;
            e.Handled = true; // don't want this click event to propagate any further
            
            // get X position in respect to this Control
            _mouseX_dragStart = e.GetPosition(this).X;
        }

        static NumberBox()
        {
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.frameworkelement.defaultstylekey?view=windowsdesktop-10.0
            // custom style key, so that WPF knows what to apply.
            DefaultStyleKeyProperty.OverrideMetadata(typeof(NumberBox), new FrameworkPropertyMetadata(typeof(NumberBox)));
        }
    }
}
