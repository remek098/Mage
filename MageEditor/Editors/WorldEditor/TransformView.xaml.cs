using MageEditor.Components;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
// using Transform = MageEditor.Components.Transform;

namespace MageEditor.Editors
{
    /// <summary>
    /// Interaction logic for TransformView.xaml
    /// </summary>
    public partial class TransformView : UserControl
    {
        private Action? _undoAction;
        private bool _propertyChanged = false;

        public TransformView()
        {
            InitializeComponent();
            Loaded += OnTransformViewLoaded;
        }

        private void OnTransformViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnTransformViewLoaded;
            ((MSTransform)DataContext).PropertyChanged += (s, e) => _propertyChanged = true;
        }

        private Action? GetAction(Func<Transform, (Transform, Vector3)> selector, 
            Action<(Transform transform, Vector3)> foreachAction)
        {
            if (!(DataContext is MSTransform vm))
            {
                _undoAction = null;
                _propertyChanged = false;
                return null;
            }
            var selection = vm.SelectedComponents.Select(x => selector(x)).ToList();
            return new Action(() =>
            {
                // set back transform positions to their old value
                selection.ForEach(x => foreachAction(x));
                (GameEntityView.Instance.DataContext as MSEntity)?.GetMSComponent<MSTransform>()?.Refresh();
            });
        }


        private Action? GetPositionAction() => GetAction((x) => (x, x.Position), (x) => x.transform.Position = x.Item2);
        private Action? GetRotationAction() => GetAction((x) => (x, x.Rotation), (x) => x.transform.Rotation = x.Item2);
        private Action? GetScaleAction() => GetAction((x) => (x, x.Scale), (x) => x.transform.Scale = x.Item2);


        private void RecordAction(Action? redoAction, string name)
        {
            if (_propertyChanged)
            {
                Debug.Assert(_undoAction != null);
                _propertyChanged = false;
                if (redoAction != null) Project.UndoRedo.Add(new UndoRedoAction(_undoAction, redoAction, name));
            }
        }

        // ---------------------------------- PREVIEW MOUSE LMB DOWN and UP

        private void OnPositionVectorBoxPreviewMouse_LMB_Down(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetPositionAction();
        }

        private void OnPositionVectorBoxPreviewMouse_LMB_Up(object sender, MouseButtonEventArgs? e)
        {
            RecordAction(GetPositionAction(), "Position changed");
        }

        private void OnRotationVectorBoxPreviewMouse_LMB_Down(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetRotationAction();
        }

        private void OnRotationVectorBoxPreviewMouse_LMB_Up(object sender, MouseButtonEventArgs? e)
        {
            RecordAction(GetRotationAction(), "Rotation changed");
        }

        private void OnScaleVectorBoxPreviewMouse_LMB_Down(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetScaleAction();
        }

        private void OnScaleVectorBoxPreviewMouse_LMB_Up(object sender, MouseButtonEventArgs? e)
        {
            RecordAction(GetScaleAction(), "Scale changed");
        }
        // ---------------------------------- END PREVIEW MOUSE LMB DOWN and UP


        // ----------------  KEYBOARD FOCUS
        private void OnPositionVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if(_propertyChanged && _undoAction != null)
            {
                OnPositionVectorBoxPreviewMouse_LMB_Up(sender, null);
            }
        }

        private void OnRotationVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (_propertyChanged && _undoAction != null)
            {
                OnRotationVectorBoxPreviewMouse_LMB_Up(sender, null);
            }
        }

        private void OnScaleVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (_propertyChanged && _undoAction != null)
            {
                OnScaleVectorBoxPreviewMouse_LMB_Up(sender, null);
            }
        }
        // ----------------  END KEYBOARD FOCUS
    }
}
