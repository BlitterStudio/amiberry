static __inline__ uae_u32 blit_func(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc, const uae_u8 mt)
{
switch(mt){
case 0x0:
	return 0;
case 0x1:
	return ~(srca | srcb | srcc);
case 0x2:
	return (srcc & ~(srca | srcb));
case 0x3:
	return ~(srca | srcb);
case 0x4:
	return (srcb & ~(srca | srcc));
case 0x5:
	return ~(srca | srcc);
case 0x6:
	return (~srca & (srcb ^ srcc));
case 0x7:
	return ~(srca | (srcb & srcc));
case 0x8:
	return (~srca & srcb & srcc);
case 0x9:
	return ~(srca | (srcb ^ srcc));
case 0xa:
	return (~srca & srcc);
case 0xb:
	return ~(srca | (srcb & ~srcc));
case 0xc:
	return (~srca & srcb);
case 0xd:
	return ~(srca | (~srcb & srcc));
case 0xe:
	return (~srca & (srcb | srcc));
case 0xf:
	return ~srca;
case 0x10:
	return (srca & ~(srcb | srcc));
case 0x11:
	return ~(srcb | srcc);
case 0x12:
	return (~srcb & (srca ^ srcc));
case 0x13:
	return ~(srcb | (srca & srcc));
case 0x14:
	return (~srcc & (srca ^ srcb));
case 0x15:
	return ~(srcc | (srca & srcb));
case 0x16:
	return (srca ^ ((srca & srcb) | (srcb ^ srcc)));
case 0x17:
	return ~(srca ^ ((srca ^ srcb) & (srca ^ srcc)));
case 0x18:
	return ((srca ^ srcb) & (srca ^ srcc));
case 0x19:
	return (srcb ^ (~srcc | (srca & srcb)));
case 0x1a:
	return (srca ^ (srcc | (srca & srcb)));
case 0x1b:
	return (srca ^ (srcc | ~(srca ^ srcb)));
case 0x1c:
	return (srca ^ (srcb | (srca & srcc)));
case 0x1d:
	return (srca ^ (srcb | ~(srca ^ srcc)));
case 0x1e:
	return (srca ^ (srcb | srcc));
case 0x1f:
	return ~(srca & (srcb | srcc));
case 0x20:
	return (srca & ~srcb & srcc);
case 0x21:
	return ~(srcb | (srca ^ srcc));
case 0x22:
	return (~srcb & srcc);
case 0x23:
	return ~(srcb | (srca & ~srcc));
case 0x24:
	return ((srca ^ srcb) & (srcb ^ srcc));
case 0x25:
	return (srca ^ (~srcc | (srca & srcb)));
case 0x26:
	return (srcb ^ (srcc | (srca & srcb)));
case 0x27:
	return ~(srca ^ (srcc & (srca ^ srcb)));
case 0x28:
	return (srcc & (srca ^ srcb));
case 0x29:
	return ~(srca ^ srcb ^ (srcc | (srca & srcb)));
case 0x2a:
	return (srcc & ~(srca & srcb));
case 0x2b:
	return ~(srca ^ ((srca ^ srcb) & (srcb ^ srcc)));
case 0x2c:
	return (srcb ^ (srca & (srcb | srcc)));
case 0x2d:
	return (srca ^ (srcb | ~srcc));
case 0x2e:
	return (srca ^ (srcb | (srca ^ srcc)));
case 0x2f:
	return ~(srca & (srcb | ~srcc));
case 0x30:
	return (srca & ~srcb);
case 0x31:
	return ~(srcb | (~srca & srcc));
case 0x32:
	return (~srcb & (srca | srcc));
case 0x33:
	return ~srcb;
case 0x34:
	return (srcb ^ (srca | (srcb & srcc)));
case 0x35:
	return (srcb ^ (srca | ~(srcb ^ srcc)));
case 0x36:
	return (srcb ^ (srca | srcc));
case 0x37:
	return ~(srcb & (srca | srcc));
case 0x38:
	return (srca ^ (srcb & (srca | srcc)));
case 0x39:
	return (srcb ^ (srca | ~srcc));
case 0x3a:
	return (srcb ^ (srca | (srcb ^ srcc)));
case 0x3b:
	return ~(srcb & (srca | ~srcc));
case 0x3c:
	return (srca ^ srcb);
case 0x3d:
	return (srca ^ (srcb | ~(srca | srcc)));
case 0x3e:
	return (srca ^ (srcb | (srca ^ (srca | srcc))));
case 0x3f:
	return ~(srca & srcb);
case 0x40:
	return (srca & srcb & ~srcc);
case 0x41:
	return ~(srcc | (srca ^ srcb));
case 0x42:
	return ((srca ^ srcc) & (srcb ^ srcc));
case 0x43:
	return (srca ^ (~srcb | (srca & srcc)));
case 0x44:
	return (srcb & ~srcc);
case 0x45:
	return ~(srcc | (srca & ~srcb));
case 0x46:
	return (srcc ^ (srcb | (srca & srcc)));
case 0x47:
	return ~(srca ^ (srcb & (srca ^ srcc)));
case 0x48:
	return (srcb & (srca ^ srcc));
case 0x49:
	return ~(srca ^ srcc ^ (srcb | (srca & srcc)));
case 0x4a:
	return (srcc ^ (srca & (srcb | srcc)));
case 0x4b:
	return (srca ^ (~srcb | srcc));
case 0x4c:
	return (srcb & ~(srca & srcc));
case 0x4d:
	return (srca ^ ((srca ^ srcb) | ~(srca ^ srcc)));
case 0x4e:
	return (srca ^ (srcc | (srca ^ srcb)));
case 0x4f:
	return ~(srca & (~srcb | srcc));
case 0x50:
	return (srca & ~srcc);
case 0x51:
	return ~(srcc | (~srca & srcb));
case 0x52:
	return (srcc ^ (srca | (srcb & srcc)));
case 0x53:
	return ~(srcb ^ (srca & (srcb ^ srcc)));
case 0x54:
	return (~srcc & (srca | srcb));
case 0x55:
	return ~srcc;
case 0x56:
	return (srcc ^ (srca | srcb));
case 0x57:
	return ~(srcc & (srca | srcb));
case 0x58:
	return (srca ^ (srcc & (srca | srcb)));
case 0x59:
	return (srcc ^ (srca | ~srcb));
case 0x5a:
	return (srca ^ srcc);
case 0x5b:
	return (srca ^ (srcc | ~(srca | srcb)));
case 0x5c:
	return (srcc ^ (srca | (srcb ^ srcc)));
case 0x5d:
	return ~(srcc & (srca | ~srcb));
case 0x5e:
	return (srca ^ (srcc | (srca ^ (srca | srcb))));
case 0x5f:
	return ~(srca & srcc);
case 0x60:
	return (srca & (srcb ^ srcc));
case 0x61:
	return ~(srcb ^ srcc ^ (srca | (srcb & srcc)));
case 0x62:
	return (srcc ^ (srcb & (srca | srcc)));
case 0x63:
	return (srcb ^ (~srca | srcc));
case 0x64:
	return (srcb ^ (srcc & (srca | srcb)));
case 0x65:
	return (srcc ^ (~srca | srcb));
case 0x66:
	return (srcb ^ srcc);
case 0x67:
	return (srcb ^ (srcc | ~(srca | srcb)));
case 0x68:
	return ((srca & srcb) ^ (srcc & (srca | srcb)));
case 0x69:
	return ~(srca ^ srcb ^ srcc);
case 0x6a:
	return (srcc ^ (srca & srcb));
case 0x6b:
	return ~(srca ^ srcb ^ (srcc & (srca | srcb)));
case 0x6c:
	return (srcb ^ (srca & srcc));
case 0x6d:
	return ~(srca ^ srcc ^ (srcb & (srca | srcc)));
case 0x6e:
	return ((~srca & srcb) | (srcb ^ srcc));
case 0x6f:
	return (~srca | (srcb ^ srcc));
case 0x70:
	return (srca & ~(srcb & srcc));
case 0x71:
	return ~(srca ^ ((srca ^ srcb) | (srca ^ srcc)));
case 0x72:
	return (srcb ^ (srcc | (srca ^ srcb)));
case 0x73:
	return ~(srcb & (~srca | srcc));
case 0x74:
	return (srcc ^ (srcb | (srca ^ srcc)));
case 0x75:
	return ~(srcc & (~srca | srcb));
case 0x76:
	return (srcb ^ (srcc | (srca ^ (srca & srcb))));
case 0x77:
	return ~(srcb & srcc);
case 0x78:
	return (srca ^ (srcb & srcc));
case 0x79:
	return ~(srcb ^ srcc ^ (srca & (srcb | srcc)));
case 0x7a:
	return ((srca & ~srcb) | (srca ^ srcc));
case 0x7b:
	return (~srcb | (srca ^ srcc));
case 0x7c:
	return ((srca ^ srcb) | (srca & ~srcc));
case 0x7d:
	return (~srcc | (srca ^ srcb));
case 0x7e:
	return ((srca ^ srcb) | (srca ^ srcc));
case 0x7f:
	return ~(srca & srcb & srcc);
case 0x80:
	return (srca & srcb & srcc);
case 0x81:
	return ~((srca ^ srcb) | (srca ^ srcc));
case 0x82:
	return (srcc & ~(srca ^ srcb));
case 0x83:
	return (srca ^ (~srcb | (srca & ~srcc)));
case 0x84:
	return (srcb & ~(srca ^ srcc));
case 0x85:
	return (srca ^ (~srcc | (srca & ~srcb)));
case 0x86:
	return (srcb ^ srcc ^ (srca & (srcb | srcc)));
case 0x87:
	return ~(srca ^ (srcb & srcc));
case 0x88:
	return (srcb & srcc);
case 0x89:
	return (srcb ^ (~srcc & (~srca | srcb)));
case 0x8a:
	return (srcc & (~srca | srcb));
case 0x8b:
	return (srca ^ (~srcb | (srca ^ srcc)));
case 0x8c:
	return (srcb & (~srca | srcc));
case 0x8d:
	return (srca ^ (~srcc | (srca ^ srcb)));
case 0x8e:
	return (srca ^ ((srca ^ srcb) | (srca ^ srcc)));
case 0x8f:
	return (~srca | (srcb & srcc));
case 0x90:
	return (srca & ~(srcb ^ srcc));
case 0x91:
	return (srcb ^ (~srcc | (~srca & srcb)));
case 0x92:
	return (srca ^ srcc ^ (srcb & (srca | srcc)));
case 0x93:
	return ~(srcb ^ (srca & srcc));
case 0x94:
	return (srca ^ srcb ^ (srcc & (srca | srcb)));
case 0x95:
	return ~(srcc ^ (srca & srcb));
case 0x96:
	return (srca ^ srcb ^ srcc);
case 0x97:
	return (srca ^ srcb ^ (srcc | ~(srca | srcb)));
case 0x98:
	return (srcb ^ (~srcc & (srca | srcb)));
case 0x99:
	return ~(srcb ^ srcc);
case 0x9a:
	return (srcc ^ (srca & ~srcb));
case 0x9b:
	return ~(srcb ^ (srcc & (srca | srcb)));
case 0x9c:
	return (srcb ^ (srca & ~srcc));
case 0x9d:
	return ~(srcc ^ (srcb & (srca | srcc)));
case 0x9e:
	return (srcb ^ srcc ^ (srca | (srcb & srcc)));
case 0x9f:
	return ~(srca & (srcb ^ srcc));
case 0xa0:
	return (srca & srcc);
case 0xa1:
	return (srca ^ (~srcc & (srca | ~srcb)));
case 0xa2:
	return (srcc & (srca | ~srcb));
case 0xa3:
	return (srcb ^ (~srca | (srcb ^ srcc)));
case 0xa4:
	return (srca ^ (~srcc & (srca | srcb)));
case 0xa5:
	return ~(srca ^ srcc);
case 0xa6:
	return (srcc ^ (~srca & srcb));
case 0xa7:
	return ~(srca ^ (srcc & (srca | srcb)));
case 0xa8:
	return (srcc & (srca | srcb));
case 0xa9:
	return ~(srcc ^ (srca | srcb));
case 0xaa:
	return srcc;
case 0xab:
	return (srcc | ~(srca | srcb));
case 0xac:
	return (srcb ^ (srca & (srcb ^ srcc)));
case 0xad:
	return ~(srcc ^ (srca | (srcb & srcc)));
case 0xae:
	return (srcc | (~srca & srcb));
case 0xaf:
	return (~srca | srcc);
case 0xb0:
	return (srca & (~srcb | srcc));
case 0xb1:
	return ~(srca ^ (srcc | (srca ^ srcb)));
case 0xb2:
	return (srca ^ ((srca ^ srcc) & (srcb ^ srcc)));
case 0xb3:
	return (~srcb | (srca & srcc));
case 0xb4:
	return (srca ^ (srcb & ~srcc));
case 0xb5:
	return ~(srcc ^ (srca & (srcb | srcc)));
case 0xb6:
	return (srca ^ srcc ^ (srcb | (srca & srcc)));
case 0xb7:
	return ~(srcb & (srca ^ srcc));
case 0xb8:
	return (srca ^ (srcb & (srca ^ srcc)));
case 0xb9:
	return ~(srcc ^ (srcb | (srca & srcc)));
case 0xba:
	return (srcc | (srca & ~srcb));
case 0xbb:
	return (~srcb | srcc);
case 0xbc:
	return ((srca ^ srcb) | (srca & srcc));
case 0xbd:
	return ((srca ^ srcb) | ~(srca ^ srcc));
case 0xbe:
	return (srcc | (srca ^ srcb));
case 0xbf:
	return (srcc | ~(srca & srcb));
case 0xc0:
	return (srca & srcb);
case 0xc1:
	return (srca ^ (~srcb & (srca | ~srcc)));
case 0xc2:
	return (srca ^ (~srcb & (srca | srcc)));
case 0xc3:
	return ~(srca ^ srcb);
case 0xc4:
	return (srcb & (srca | ~srcc));
case 0xc5:
	return ~(srcb ^ (srca | (srcb ^ srcc)));
case 0xc6:
	return (srcb ^ (~srca & srcc));
case 0xc7:
	return ~(srca ^ (srcb & (srca | srcc)));
case 0xc8:
	return (srcb & (srca | srcc));
case 0xc9:
	return ~(srcb ^ (srca | srcc));
case 0xca:
	return (srcc ^ (srca & (srcb ^ srcc)));
case 0xcb:
	return ~(srcb ^ (srca | (srcb & srcc)));
case 0xcc:
	return srcb;
case 0xcd:
	return (srcb | ~(srca | srcc));
case 0xce:
	return (srcb | (~srca & srcc));
case 0xcf:
	return (~srca | srcb);
case 0xd0:
	return (srca & (srcb | ~srcc));
case 0xd1:
	return ~(srca ^ (srcb | (srca ^ srcc)));
case 0xd2:
	return (srca ^ (~srcb & srcc));
case 0xd3:
	return ~(srcb ^ (srca & (srcb | srcc)));
case 0xd4:
	return (srca ^ ((srca ^ srcb) & (srcb ^ srcc)));
case 0xd5:
	return (~srcc | (srca & srcb));
case 0xd6:
	return (srca ^ srcb ^ (srcc | (srca & srcb)));
case 0xd7:
	return ~(srcc & (srca ^ srcb));
case 0xd8:
	return (srca ^ (srcc & (srca ^ srcb)));
case 0xd9:
	return ~(srcb ^ (srcc | (srca & srcb)));
case 0xda:
	return ((srca & srcb) | (srca ^ srcc));
case 0xdb:
	return ~((srca ^ srcb) & (srcb ^ srcc));
case 0xdc:
	return (srcb | (srca & ~srcc));
case 0xdd:
	return (srcb | ~srcc);
case 0xde:
	return (srcb | (srca ^ srcc));
case 0xdf:
	return (srcb | ~(srca & srcc));
case 0xe0:
	return (srca & (srcb | srcc));
case 0xe1:
	return ~(srca ^ (srcb | srcc));
case 0xe2:
	return (srcc ^ (srcb & (srca ^ srcc)));
case 0xe3:
	return ~(srca ^ (srcb | (srca & srcc)));
case 0xe4:
	return (srcb ^ (srcc & (srca ^ srcb)));
case 0xe5:
	return ~(srca ^ (srcc | (srca & srcb)));
case 0xe6:
	return ((srca & srcb) | (srcb ^ srcc));
case 0xe7:
	return ~((srca ^ srcb) & (srca ^ srcc));
case 0xe8:
	return (srca ^ ((srca ^ srcb) & (srca ^ srcc)));
case 0xe9:
	return (srca ^ srcb ^ (~srcc | (srca & srcb)));
case 0xea:
	return (srcc | (srca & srcb));
case 0xeb:
	return (srcc | ~(srca ^ srcb));
case 0xec:
	return (srcb | (srca & srcc));
case 0xed:
	return (srcb | ~(srca ^ srcc));
case 0xee:
	return (srcb | srcc);
case 0xef:
	return (~srca | srcb | srcc);
case 0xf0:
	return srca;
case 0xf1:
	return (srca | ~(srcb | srcc));
case 0xf2:
	return (srca | (~srcb & srcc));
case 0xf3:
	return (srca | ~srcb);
case 0xf4:
	return (srca | (srcb & ~srcc));
case 0xf5:
	return (srca | ~srcc);
case 0xf6:
	return (srca | (srcb ^ srcc));
case 0xf7:
	return (srca | ~(srcb & srcc));
case 0xf8:
	return (srca | (srcb & srcc));
case 0xf9:
	return (srca | ~(srcb ^ srcc));
case 0xfa:
	return (srca | srcc);
case 0xfb:
	return (srca | ~srcb | srcc);
case 0xfc:
	return (srca | srcb);
case 0xfd:
	return (srca | srcb | ~srcc);
case 0xfe:
	return (srca | srcb | srcc);
case 0xff:
	return 0xFFFFFFFF;
}
return 0;
}
