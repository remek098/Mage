using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.Components
{
    [DataContract]
    class Script : Component
    {
        public string _name;

        [DataMember]
        public string Name
        {
            get => _name;
            set
            {
                if(_name  != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        public override IMSComponent GetMultiSelectionComponent(MSEntity msEntity) => new MSScript(msEntity);

        /// <summary>
        /// writes script name's:
        /// <para> 1) Length of UTF8 encoded script name (string) </para>
        /// <para> 2) Writes script name's (string) bytes encoded with UTF8 </para>
        /// </summary>
        /// <param name="bw"></param>
        public override void WriteToBinary(BinaryWriter bw)
        {
            var nameBytes = Encoding.UTF8.GetBytes(Name);
            bw.Write(nameBytes.Length);
            bw.Write(nameBytes);
        }

        public Script(GameEntity owner) : base(owner)
        {
        }
    }


    sealed class MSScript : MSComponent<Script>
    {
        public string? _name;
        public string? Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        protected override bool UpdateComponents(string propertyName)
        {
            if(propertyName == nameof(Name))
            {
                if(_name != null) SelectedComponents.ForEach(c => c.Name = _name);
                return true;
            }

            return false;
        }

        protected override bool UpdateMSComponent()
        {
            Name = MSEntity.GetMixedValue(SelectedComponents, new Func<Script, string>(x => x.Name));
            return true;
        }

        public MSScript(MSEntity msEntity) : base(msEntity)
        {
            Refresh();
        }
    }
}
