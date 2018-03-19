using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Generator
{

    public class PortableBinaryWriter : BinaryWriter
    {
        public PortableBinaryWriter(Stream output) : base(output) { }
        public override void Write(ulong value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        public override void Write(uint value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        public override void Write(ushort value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        public override void Write(long value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        public override void Write(int value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        public override void Write(short value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        private void WriteBigEndian(byte[] nativeIntBytes)
        {
            if (BitConverter.IsLittleEndian)
                Array.Reverse(nativeIntBytes);
            Write(nativeIntBytes);
        }
    }

}
