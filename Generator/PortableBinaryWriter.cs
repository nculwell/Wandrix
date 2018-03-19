using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Generator
{

    /// <summary> Binary writer that writes integers in big-endian order. </summary>
    public class PortableBinaryWriter : BinaryWriter
    {
        /// <summary> Open file 'path' for writing. </summary>
        public PortableBinaryWriter(string path) : base(new FileStream(path, FileMode.Open, FileAccess.Write)) { }
        /// <summary> Use stream for writing. </summary>
        public PortableBinaryWriter(Stream output) : base(output) { }
        /// <summary> Write 64-bit unsigned int in big-endian order. </summary>
        public override void Write(ulong value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write 32-bit unsigned int in big-endian order. </summary>
        public override void Write(uint value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write 16-bit unsigned int in big-endian order. </summary>
        public override void Write(ushort value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write 64-bit signed int in big-endian order. </summary>
        public override void Write(long value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write 32-bit signed int in big-endian order. </summary>
        public override void Write(int value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write 16-bit signed int in big-endian order. </summary>
        public override void Write(short value) { WriteBigEndian(BitConverter.GetBytes(value)); }
        /// <summary> Write int in big-endian order, swapping order if necessary. </summary>
        private void WriteBigEndian(byte[] nativeIntBytes)
        {
            if (BitConverter.IsLittleEndian)
                Array.Reverse(nativeIntBytes);
            Write(nativeIntBytes);
        }
        /// <summary> Write fixed-length string, preceded by 32-bit length. </summary>
        public void Write(int fixedLength, string str)
        {
            if (str.Length > fixedLength)
                throw new ArgumentException($"String length ({str.Length}) exceeds fixed length ({fixedLength}).");
            Write(fixedLength);
            Write(str);
            for (int i = str.Length; i < fixedLength; ++i)
                Write('\0');
        }
    }

}
