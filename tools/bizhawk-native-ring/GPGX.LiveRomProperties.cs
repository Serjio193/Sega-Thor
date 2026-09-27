using System.Globalization;

namespace BizHawk.Emulation.Cores.Consoles.Sega.gpgx
{
	public partial class GPGX
	{
		public void RomPropertiesEnable(bool enabled)
			=> Core.gpgx_rom_properties_enable(enabled ? 1u : 0u);

		public bool RomPropertiesClear()
			=> Core.gpgx_rom_properties_clear() != 0;

		public uint RomPropertiesSize()
			=> Core.gpgx_rom_properties_size();

		public ulong RomPropertiesChangedBytes()
			=> Core.gpgx_rom_properties_changed_bytes();

		public ulong RomPropertiesChangeOperations()
			=> Core.gpgx_rom_properties_change_operations();

		public string RomPropertiesCopyHex(uint offset, uint capacity)
		{
			if (capacity == 0 || capacity > 4096) return string.Empty;
			var values = new ushort[checked((int)capacity)];
			uint copied = Core.gpgx_rom_properties_copy(offset, values,
				(uint)values.Length);
			if (copied != values.Length) return string.Empty;
			var chars = new char[checked((int)copied * 4)];
			for (int i = 0; i < copied; ++i)
				values[i].ToString("x4", CultureInfo.InvariantCulture).CopyTo(
					0, chars, i * 4, 4);
			return new string(chars);
		}
	}
}
