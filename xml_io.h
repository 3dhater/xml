#ifndef _XML_IO_H_
#define _XML_IO_H_

#ifdef GAME_TOOL
#include <filesystem>
#define kkCreate(type) Game_Create<type>
#define kkDestroy(x) Game_Destroy(x)
#define kkXMLString Game_String
#define kkXMLStringA Game_StringA
#define kkArray Game_Array
#define kkFileExist(x) std::filesystem::exists(x)
#else
#include <filesystem>
#define kkDestroy(x) delete x
#define kkXMLString std::u16string
#define kkXMLStringA std::string
#define kkArray std::vector
#define kkFileExist(x) std::filesystem::exists(x)
#define kkCreate(type) new type
#endif


template<typename _type>
class kkPtr{ // ....
	_type * m_ptr = nullptr;
public:
	kkPtr(){}
	~kkPtr(){
		if( m_ptr ){
			kkDestroy( m_ptr );
			m_ptr = nullptr;
		}
	}
	kkPtr( _type * p ):m_ptr( p ){}
	_type * ptr(){return m_ptr;}
	_type* operator->() const{return m_ptr;}
	void operator=( _type* p ){m_ptr = p;}
};
enum class kkTextFileFormat : unsigned int{
	UTF_8,
	UTF_16,
	UTF_32
};
enum class kkTextFileEndian : unsigned int{
	Little, //	0xFFFE0000
	Big // 0x0000FEFF, not implemented !
};
struct kkTextFileInfo{
	kkTextFileFormat m_format;
	kkTextFileEndian m_endian;
	bool m_hasBOM;
};
enum class kkFileSeekPos : unsigned int{
	Begin,
	Current,
	End
};
enum class kkFileAccessMode : unsigned int
{
	Read,
	Write,
	Both,
	Append
};
enum class kkFileMode : unsigned int
{
	Text,
	Binary
};
enum class kkFileShareMode : unsigned int
{
	None,
	Delete,
	Read,
	Write
};
enum class kkFileAction : unsigned int
{
	Open,
	Open_new,
};
enum class kkFileAttribute : unsigned int
{
	Normal,
	Hidden,
	Readonly
};

#if defined(WIN32) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
struct kkFile{
	kkTextFileInfo	m_textInfo;
	bool			m_isTextFile = false;
	HANDLE		m_handle = nullptr;
	unsigned long long			m_pointerPosition = 0;
	DWORD		m_desiredAccess = 0;
	kkFile( const kkXMLString& fileName,
		kkFileMode mode,
		kkFileAccessMode access,
		kkFileAction action,
		kkFileShareMode EFSM = kkFileShareMode::Read,
		unsigned int EFA = 0 )
	{
		if( mode == kkFileMode::Text )
			m_isTextFile = true;
		switch( access ){
		case kkFileAccessMode::Read:
			m_desiredAccess |= GENERIC_READ;
			break;
		case kkFileAccessMode::Write:
			m_desiredAccess |= GENERIC_WRITE;
			break;
		case kkFileAccessMode::Both:
			m_desiredAccess |= GENERIC_READ | GENERIC_WRITE;
			break;
		case kkFileAccessMode::Append:
			m_desiredAccess |= FILE_APPEND_DATA;
			break;
		}
		DWORD ShareMode = 0;
		switch( EFSM ){
		default:
		case kkFileShareMode::None:
			break;
		case kkFileShareMode::Delete:
			ShareMode = FILE_SHARE_DELETE;
			break;
		case kkFileShareMode::Read:
			ShareMode = FILE_SHARE_READ;
			break;
		case kkFileShareMode::Write:
			ShareMode = FILE_SHARE_WRITE;
			break;
		}
		DWORD CreationDisposition = 0;
		switch( action ){
		case kkFileAction::Open:
			CreationDisposition = OPEN_ALWAYS;
			break;
		case kkFileAction::Open_new:
			CreationDisposition = CREATE_ALWAYS;
			break;
		}
		DWORD FlagsAndAttributes = 0;
		if( EFA & (unsigned int)kkFileAttribute::Normal ){
			FlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
		}else{
			if( EFA & (unsigned int)kkFileAttribute::Hidden ){
				FlagsAndAttributes |= FILE_ATTRIBUTE_HIDDEN;
			}
			if( EFA & (unsigned int)kkFileAttribute::Readonly ){
				FlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
			}
		}
		m_handle = CreateFileW( (wchar_t*)fileName.data(), m_desiredAccess, ShareMode, NULL,
			CreationDisposition, FlagsAndAttributes, NULL );
	//	if( !m_handle )
			//printWarning( u"Can not create file [%s], error code[%u]",fileName.data(), GetLastError() );
	}
	~kkFile(){
		if( m_handle ){
			CloseHandle( m_handle );
			m_handle = nullptr;
		}
	}
	kkTextFileInfo&	getTextFileInfo(){return m_textInfo;}
	void			setTextFileInfo( const kkTextFileInfo& info ){m_textInfo = info;}
	unsigned int	write( unsigned char * data, unsigned int size ){
		assert(m_desiredAccess & GENERIC_WRITE);
		if( !m_handle ){
			//printWarning( u"Can not write text to file. m_handle == nullptr" );
			return 0;
		}
		DWORD bytesWritten = 0;
		if( WriteFile( m_handle, data, size, &bytesWritten, NULL ) == FALSE )
		{
			//printWarning( u"Can not write text to file. Error code [%u]", GetLastError() );
		}else
			m_pointerPosition += size;
		return bytesWritten;
	}
	void	write( const kkXMLString& string ){
		if( !m_handle ){
			fprintf( stderr, "Can not write text to file. m_handle == nullptr\n" );
			return;
		}
		DWORD bytesWritten;
		if( WriteFile( m_handle, string.c_str(), (DWORD)string.size() * sizeof(char16_t), &bytesWritten, NULL ) == FALSE )
			fprintf( stderr, "Can not write text to file. Error code [%u]\n", GetLastError() );
		else
			m_pointerPosition += string.size() * sizeof(char16_t);
	}
	void	write( const kkXMLStringA& string ){
		assert( m_isTextFile );
		if( !m_handle ){
			fprintf( stderr, "Can not write text to file. m_handle == nullptr\n" );
			return;
		}
		DWORD bytesWritten;
		if( WriteFile( m_handle, string.c_str(), (DWORD)string.size(), &bytesWritten, NULL ) == FALSE ){
			fprintf( stderr, "Can not write text to file. Error code [%u]\n", GetLastError() );
		}else
			m_pointerPosition += string.size();
	}
	void	flush(){
		if( m_handle )
			FlushFileBuffers( m_handle );
	}
	unsigned long long	read( unsigned char * data, unsigned long long size ){
		assert( m_desiredAccess & GENERIC_READ );
		if( m_handle ){
			DWORD readBytesNum = 0;
			if( ReadFile(	m_handle,
							data, (DWORD)size,
							&readBytesNum,
							NULL
				) == FALSE ){
				//printWarning( u"Can not read file. Error code [%u]", GetLastError() );
			}
			return readBytesNum;
		}
		return 0;
	}
	unsigned long long		size(){
		if( !m_handle ){
			//printWarning( u"Can not get file size. m_handle == nullptr" );
			return 0;
		}
		return static_cast<unsigned long long>( GetFileSize( m_handle, NULL ) );
	}
	unsigned long long		tell(){return m_pointerPosition;}
	void		seek( unsigned long long distance, kkFileSeekPos pos ){
		if( (m_desiredAccess & GENERIC_READ) || (m_desiredAccess & GENERIC_WRITE) ){
			if( m_handle ){
				LARGE_INTEGER li;
				li.QuadPart = distance;
				li.LowPart = SetFilePointer( m_handle, li.LowPart, (PLONG)&li.HighPart, (DWORD)pos );
				m_pointerPosition = li.QuadPart;

			}
		}
	}
};
#else
#erro Only for windows
#endif

namespace xmlutil
{
	inline kkFile* openFileForReadText( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Text, kkFileAccessMode::Read, kkFileAction::Open );
	}
		
	inline kkFile* createFileForReadText( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Text, kkFileAccessMode::Read, kkFileAction::Open_new );
	}

	inline kkFile* openFileForWriteText( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Text, kkFileAccessMode::Append, kkFileAction::Open );
	}
		
	inline	kkFile* createFileForWriteText( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Text, kkFileAccessMode::Write,kkFileAction::Open_new );
	}

	inline kkFile* openFileForReadBin( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Binary, kkFileAccessMode::Read, kkFileAction::Open );
	}

	inline kkFile* openFileForReadBinShared( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Binary, kkFileAccessMode::Read, kkFileAction::Open, kkFileShareMode::Read );
	}

	inline kkFile* openFileForWriteBin( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Binary, kkFileAccessMode::Write, kkFileAction::Open );
	}

	inline kkFile* createFileForWriteBin( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Binary, kkFileAccessMode::Write, kkFileAction::Open_new );
	}

	inline kkFile* createFileForWriteBinShared( const kkXMLString& fileName ){
		return kkCreate(kkFile)( fileName, kkFileMode::Binary, kkFileAccessMode::Write, kkFileAction::Open_new, kkFileShareMode::Read );
	}
	template<typename char_type>
	bool isDigit( char_type c ){
		if( c < 0x7B ){
			if( c >= (char_type)'0' && c <= (char_type)'9' )return true;
		}
		return false;
	}
	template<typename char_type>
	bool isSpace( char_type c ){
		if( c == (char_type)' ' ) return true;
		if( c == (char_type)'\r' ) return true;
		if( c == (char_type)'\n' ) return true;
		if( c == (char_type)'\t' ) return true;
		return false;
	}
	template<typename char_type>
	bool isAlpha( char_type c ){
		if( c < 0x7B ){
			if( (c >= (char_type)'a' && c <= (char_type)'z')
				|| (c >= (char_type)'A' && c <= (char_type)'Z') )
				return true;
		}else if( c >= 0xC0 && c <= 0x2AF ){
			return true;
		}else if( c >= 0x370 && c < 0x374 ){
			return true;
		}else if( c >= 0x376 && c < 0x378 ){
			return true;
		}else if( c >= 0x376 && c < 0x378 ){
			return true;
		}else if( c == 0x37F || c == 0x386 ){
			return true;
		}else if( c > 0x387 && c < 0x38B ){
			return true;
		}else if( c == 0x38C ){
			return true;
		}else if( c > 0x38D && c < 0x3A2 ){
			return true;
		}else if( c > 0x3A2 && c < 0x482 ){
			return true;
		}else if( c > 0x489 && c < 0x530 ){
			return true;
		}else if( c > 0x530 && c < 0x557 ){
			return true;
		}else if( c > 0x560 && c < 0x588 ){
			return true;
		}else if( c >= 0x5D0 && c < 0x5EB ){
			return true;
		}
		return false;
	}
	inline void string_UTF16_to_UTF8(kkXMLString& utf16, kkXMLStringA& utf8 ){
		size_t sz = utf16.size();
		for( size_t i = 0u; i < sz; ++i ){
			char16_t ch16 = utf16[ i ];
			if( ch16 < 0x80 ){
				utf8 += (char)ch16;
			}else if( ch16 < 0x800 ){
				utf8 += (char)((ch16>>6)|0xc0);
				utf8 += (char)((ch16&0x3f)|0x80);
			}
		}
	}
	inline void string_UTF8_to_UTF16( kkXMLString& utf16, kkXMLStringA& utf8 ){
		std::vector<unsigned int> unicode;
		unsigned int i = 0u;
		auto sz = utf8.size();
		while( i < sz ){
			unsigned int uni = 0u;
			unsigned int todo = 0u;
//				bool error = false;
			unsigned char ch = utf8[i++];
			if( ch <= 0x7F ){
				uni = ch;
				todo = 0;
			}else if( ch <= 0xBF ){
				//throw std::logic_error("not a UTF-8 string");
			}else if ( ch <= 0xDF ){
				uni = ch&0x1F;
				todo = 1;
			}else if( ch <= 0xEF ){
				uni = ch&0x0F;
				todo = 2;
			}else if( ch <= 0xF7 ){
				uni = ch&0x07;
				todo = 3;
			}else{
				//throw std::logic_error("not a UTF-8 string");
			}
			for( unsigned int j = 0; j < todo; ++j ){
				//if( i == utf8.size() )
					//throw std::logic_error("not a UTF-8 string");
				unsigned char ch2 = utf8[i++];
				//if( ch < 0x80 || ch > 0xBF )
					//throw std::logic_error("not a UTF-8 string");
				uni <<= 6;
				uni += ch2 & 0x3F;
			}
			//if( uni >= 0xD800 && uni <= 0xDFFF )
				//throw std::logic_error("not a UTF-8 string");
			//if( uni > 0x10FFFF )
				//throw std::logic_error("not a UTF-8 string");
			unicode.push_back(uni);
		}
		auto sz2 = unicode.size();
		for( unsigned int o = 0u; o < sz2; ++o ){
			unsigned int uni = unicode[o];
			if( uni <= 0xFFFF ){
				utf16 += (char16_t)uni;
			}else{
				uni -= 0x10000;
				utf16 += (wchar_t)((uni >> 10) + 0xD800);
				utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
			}
		}
	}
	inline bool readTextFromFileForUnicode( const kkXMLString& fileName, kkXMLString& utf16 )
	{
		kkFile* file = xmlutil::openFileForReadBin( fileName );
		if( !file ){
			kkDestroy(file);
			return false;
		}
		size_t sz = (size_t)file->size();
		if( sz < 4 ){
			kkDestroy(file);
			return false;
		}
		unsigned char bom[ 3u ];
		file->read( bom, 3u );
		file->seek( 0u, kkFileSeekPos::Begin );
		bool isUTF8 = false;
		bool isBE = false;
		if( bom[ 0u ] == 0xEF ){
			file->seek( 3u, kkFileSeekPos::Begin );
			isUTF8 = true;
			sz -= 3u;
		}else if( bom[ 0u ] == 0xFE ){ // utf16 BE
			file->seek( 2u, kkFileSeekPos::Begin );
			isBE = true;
			sz -= 2u;
		}else if( bom[ 0u ] == 0xFF ){
			file->seek( 2u, kkFileSeekPos::Begin );
			sz -= 2u;
		}else{
			// else - utf8 w/o bom
			isUTF8 = true;
		}

		unsigned char* buf = new unsigned char[sz];
		file->read( buf, sz );

		kkXMLStringA textBytes;
		textBytes.reserve( (unsigned int)sz + 2 );
		
		for(unsigned long long i = 0; i < sz; ++i)
		{
			textBytes.push_back( (char)buf[i] );
		}
		//textBytes.setSize( (unsigned int)sz );
		//textBytes.data()[textBytes.size()] = 0;

		if( !isUTF8 ){
			union{
				char16_t unicode;
				char b[ 2u ];
			}un;
			for( unsigned int i = 0u; i < sz; i += 2u ){
				/*char16_t ch16 = textBytes[ i ];
				if( isBE ){
					ch16 <<= kkConst8U;
					ch16 |= textBytes[ i + 1u ];
				}else{
					char16_t ch16_2 = textBytes[ i + 1u ];
					ch16_2 <<= kkConst8U;
					ch16 |= ch16_2;
				}*/

				if( isBE ){
					un.b[ 0u ] = textBytes[ i + 1u ];
					un.b[ 1u ] = textBytes[ i ];
				}else{
					un.b[ 0u ] = textBytes[ i ];
					un.b[ 1u ] = textBytes[ i + 1u ];
				}

				utf16 += un.unicode;
			}

		}else
		{
			xmlutil::string_UTF8_to_UTF16( utf16, textBytes );
		}
		kkDestroy(file);
		return true;
	}
	template<typename Type>
	inline void stringTrimSpace( Type& str ){
		while( true ){
			if( isSpace( str[0] ) ) str.erase(0,1);
			else  break;
		}
		while( true ){
			if( isSpace(str[str.size() - 1u]) )str.pop_back();
			else break;
		}
	}
	inline void stringReplaseSubString( kkXMLString& source, const kkXMLString& target, const kkXMLString& text )
	{
		kkXMLString result;
		u32 source_sz = source.size();
		u32 target_sz = target.size();
		u32 text_sz   = text.size();
		for( u32 i = 0u; i < source_sz; ++i ){
			if( (source_sz - i) < target_sz ){
				for( u32 i2 = i; i2 < source_sz; ++i2 ){
					result += source[ i2 ];
				}
				break;
			}
			bool comp = false;
			for( u32 o = 0u; o < target_sz; ++o ){
				if( source[ i + o ] == target[ o ] ){
					if( !comp ) comp = true;
				}else{
					comp = false;
					break;
				}
			}
			if( comp ){
				for( u32 o = 0u; o < text_sz; ++o ){
					result += text[ o ];
				}
				i += target_sz - 1u;
			}else result += source[ i ];
		}
		if( result.size() ){
			source.clear();
			source.assign( result );
		}
	}
}

enum class kkXPathTokenType : unsigned int{
	Slash,
	Double_slash,
	Name,
	Equal,			// price=9.80
	Not_equal,		// price!=9.80
	More,			// price>9.80
	Less,			// price<9.80
	More_eq,		// price>=9.80
	Less_eq,		// price<=9.80
	Apos,
	Number,
	Comma, //,
	Function,
	Function_open,  //(
	Function_close, //)
	Attribute,
	Bit_or,			// //book | //cd
	Sq_open,		// [
	Sq_close,		// ]
	Div,			// 8 div 4
	Mod,			// 5 mod 2
	Add,			// 6 + 4
	Sub,			// 6 - 4
	Mul,			// 6 * 4
	And,			// price>9.00 and price<9.90
	Or,				// price=9.80 or price=9.70,
	Axis_namespace,	//::
	Axis,
	NONE = 0xFFFFFFF
};
enum class kkXPathAxis : unsigned int{
	Ancestor,			// parent, grandparent, etc.
	Ancestor_or_self,	// parent, grandparent, etc. + current node
	Attribute,
	Child,
	Descendant,			// children, grandchildren, etc.
	Descendant_or_self,	// children, grandchildren, etc. + current node
	Following,
	Following_sibling,
	Namespace,
	Parent,
	Preceding,
	Preceding_sibling,
	Self,
	NONE = 0xFFFFFFF
};
struct kkXPathToken{
	kkXPathToken(){}
	kkXPathToken( kkXPathTokenType type,kkXMLString string,float number )
	: m_type( type ),m_axis(kkXPathAxis::NONE),m_string( string ),m_number( number ){}
	kkXPathTokenType    m_type = kkXPathTokenType::NONE;
	kkXPathAxis         m_axis = kkXPathAxis::NONE;
	kkXMLString         m_string;
	float          m_number = 0.f;
};
struct kkXMLAttribute{
	kkXMLAttribute(){}
	kkXMLAttribute( const kkXMLString& Name,const kkXMLString& Value ):name( Name ),value( Value ){}
	kkXMLString name;
	kkXMLString value;
};
struct kkXMLNode{
	kkXMLNode(){}
	kkXMLNode( const kkXMLString& Name ):name( Name ){}
	kkXMLNode( const kkXMLNode& node ){name = node.name;text = node.text;attributeList = node.attributeList;nodeList = node.nodeList;}
	~kkXMLNode(){clear();}
	kkXMLString name;
	kkXMLString text;
	kkArray<kkXMLAttribute*> attributeList;
	kkArray<kkXMLNode*> nodeList;
	void addAttribute( const kkXMLString& Name,const kkXMLString& Value ){
		attributeList.push_back( new kkXMLAttribute( Name, Value ) );
	}
	void addAttribute( kkXMLAttribute* a ){
		attributeList.push_back( a );
	}
	void addNode( kkXMLNode* node ){
		nodeList.push_back( node );
	}
	kkXMLNode& operator=( const kkXMLNode& node ){
		name = node.name;
		text = node.text;
		attributeList = node.attributeList;
		nodeList = node.nodeList;
		return *this;
	}
	kkXMLAttribute*	getAttribute( const kkXMLString& Name ){
		unsigned int sz = (unsigned int)attributeList.size();
		for( unsigned int i = 0; i < sz; ++i ){
			if( attributeList[ i ]->name == Name )
				return attributeList[ i ];
		}
		return nullptr;
	}
	kkXMLNode*	getNode( const kkXMLString& Name ){
		unsigned int sz = (unsigned int)nodeList.size();
		for( unsigned int i = 0; i < sz; ++i ){
			if( nodeList[ i ]->name == Name )
				return nodeList[ i ];
		}
		return nullptr;
	}
	kkArray<kkXMLNode*>	getNodes( const kkXMLString& Name ){
		kkArray<kkXMLNode*> arr;
		unsigned int sz = (unsigned int)nodeList.size();
		for( unsigned int i = 0; i < sz; ++i ){
			auto node = nodeList[ i ];
			if( node->name == Name )
			{
				arr.push_back(node);
			}
		}
		return arr;
	}
	void clear(){
		name.clear();
		text.clear();
		unsigned int sz = (unsigned int)attributeList.size();
		for( unsigned int i = 0; i < sz; ++i ){
			kkDestroy(attributeList[ i ]);
		}
		sz =  (unsigned int)nodeList.size();
		for( unsigned int i = 0; i < sz; ++i ){
			kkDestroy(nodeList[ i ]);
		}
		attributeList.clear();
		nodeList.clear();
	}
};

class kkXMLDocument{
	bool		m_isInit = false;
	kkXMLNode	m_root;
	kkXMLString	m_fileName;
	kkXMLString	m_text;

	kkXMLString m_expect_apos;
	kkXMLString m_expect_quot;
	kkXMLString m_expect_eq;
	kkXMLString m_expect_slash;
	kkXMLString m_expect_lt;
	kkXMLString m_expect_gt;
	kkXMLString m_expect_sub;
	kkXMLString m_expect_ex;
	void _initExpectStrings()
	{
		m_expect_apos = u"\'";
		m_expect_quot = u"\"";
		m_expect_eq   = u"=";
		m_expect_slash= u"/";
		m_expect_lt   = u"<";
		m_expect_gt   = u">";
		m_expect_sub  = u"-";
		m_expect_ex   = u"!";
	}

	unsigned int m_cursor = 0;
	unsigned int m_sz = 0;

	enum _token_type{
		tt_default,
		tt_string
	};
	struct _token{
		_token( kkXMLString N, unsigned int R, unsigned int C, _token_type t = _token_type::tt_default ):
			name( N ), line( R ), col( C ), type( t ){}
		kkXMLString name;
		unsigned int line;
		unsigned int col;
		_token_type type;
	};

	kkArray<_token> m_tokens;
	void getTokens(){
		char16_t * ptr = m_text.data();
		unsigned int line = 1;
		unsigned int col = 1;
		bool isString = false;
		bool stringType = false; // "
		kkXMLString str;
		unsigned int oldCol = 0;
		while( *ptr ){
			if( *ptr == u'\n' ){
				col = 0;
				++line;
			}else{
				if( !isString ){
					if( charIsSymbol( ptr ) ){
						kkXMLString tmp; tmp += *ptr;
						m_tokens.push_back( _token( tmp, line, col ) );
						if( *ptr == u'\'' ){
							oldCol = col;
							str.clear();
							isString = true;
							stringType = true;
						}else if( *ptr == u'\"' ){
							oldCol = col;
							isString = true;
							stringType = false;
							str.clear();
						}else if( *ptr == u'>' ){
							++ptr;
							++col;
							ptr = skipSpace( ptr, line, col );
							oldCol = col;
							ptr = getString( ptr, str, line, col );
							if( str.size() )
							{
								xmlutil::stringTrimSpace( str );
								decodeEnts( str );
								m_tokens.push_back( _token( str, line, oldCol ) );
								str.clear();
							}
							continue;
						}
					}
					else if( charForName( ptr ) ){
						oldCol = col;
						kkXMLString name;
						ptr = getName( ptr, name, line, col );
						m_tokens.push_back( _token( name, line, oldCol ) );
						continue;
					}
				}else{
					if( stringType ){ // '
						if( *ptr == u'\'' ){
							decodeEnts( str );
							m_tokens.push_back( _token( str, line, oldCol+1, kkXMLDocument::_token_type::tt_string ) );
							kkXMLString tmp; tmp += *ptr;
							m_tokens.push_back( _token( tmp, line, col ) );
							str.clear();
							isString = false;
							goto chponk;
						}
					}
					else{ // "
						if( *ptr == u'\"' ){
							decodeEnts( str );
							m_tokens.push_back( _token( str, line, oldCol+1, kkXMLDocument::_token_type::tt_string ) );
							kkXMLString tmp; tmp += *ptr;
							m_tokens.push_back( _token( tmp, line, col ) );
							str.clear();
							isString = false;
							goto chponk;
						}
					}
					str += *ptr;
				}
			}
	chponk:
			++col;
			++ptr;
		}
	}
	void decodeEnts( kkXMLString& str ){
		xmlutil::stringReplaseSubString( str, kkXMLString(u"&apos;"), kkXMLString(u"\'") );
		xmlutil::stringReplaseSubString( str, kkXMLString(u"&quot;"), kkXMLString(u"\"") );
		xmlutil::stringReplaseSubString( str, kkXMLString(u"&lt;"), kkXMLString(u"<") );
		xmlutil::stringReplaseSubString( str, kkXMLString(u"&kk;"), kkXMLString(u">") );
		xmlutil::stringReplaseSubString( str, kkXMLString(u"&amp;"), kkXMLString(u"&") );
	}
	char16_t * getName( char16_t * ptr, kkXMLString& outText, unsigned int& line, unsigned int& col ){
		while( *ptr ){
			if( charForName( ptr ) ) outText += *ptr;
			else return ptr; 
			++ptr;
			++col;
		}
		return ptr;
	}
	char16_t * getString( char16_t * ptr, kkXMLString& outText, unsigned int& line, unsigned int& col ){
		while( *ptr ){
			if( *ptr == u'\n' ){
				++line;
				col = 1;
				outText += *ptr;
				++ptr;
			}else if( *ptr == u'<' )
				break;
			else{
				outText += *ptr;
				++col;
				++ptr;
			}
		}
		return ptr;
	}

	char16_t * skipSpace( char16_t * ptr, unsigned int& line, unsigned int& col ){
		while( *ptr ){
			if( *ptr == u'\n' ){
				++line;
				col = 1;
				++ptr;
			}else if( (*ptr == u'\r')
				|| (*ptr == u'\t')
				|| (*ptr == u' ')){
				++col;
				++ptr;
			}else break;
		}
		return ptr;
	}

	bool charForName( char16_t * ptr ){
		char16_t c = *ptr;
		if( c > 0x80 ) return true;
		if( xmlutil::isAlpha( *ptr ) 
				|| xmlutil::isDigit( *ptr )
				|| (*ptr == u'_')
				|| (*ptr == u'.')){
			return true;
		}
		return false;
	}
	bool charForString( char16_t * ptr ){
		char16_t c = *ptr;
		if( c > 0x80 ) return true;
		if( xmlutil::isAlpha( *ptr ) 
				|| xmlutil::isDigit( *ptr )
				|| (*ptr == u'_')
				|| (*ptr == u'.')){
			return true;
		}
		return false;
	}
	bool charIsSymbol( char16_t * ptr ){
		char16_t c = *ptr;
		if( (c == u'<') || (c == u'>')
			|| (c == u'/') || (c == u'\'')
			|| (c == u'\"') || (c == u'=')
			|| (c == u'?') || (c == u'!')
			|| (c == u'-') ){
			return true;
		}
		return false;
	}
	bool analyzeTokens(){
		unsigned int sz = (unsigned int)m_tokens.size();
		if( !sz ){
			fprintf( stderr, "Empty XML\n" );
			return false;
		}
		m_cursor = 0;
		if( m_tokens[ 0 ].name == u"<" ){
			if( m_tokens[ 1 ].name == u"?" ){
				if( m_tokens[ 2 ].name == u"xml" ){
					m_cursor = 2;
					skipPrologAndDTD();
				}
			}
		}
		if( m_tokens[ m_cursor ].name == u"<" ){
			if( m_tokens[ m_cursor + 1 ].name == u"!" ){
				if( m_tokens[ m_cursor + 2 ].name == u"DOCTYPE" ) skipPrologAndDTD();
			}
		}
		return buildXMLDocument();
	}

	bool buildXMLDocument(){
		m_sz = (unsigned int)m_tokens.size();
		return getSubNode( &m_root);
	}
	bool getSubNode( kkXMLNode * node ){	
		//kkPtr<kkXMLNode> subNode = kkCreate<kkXMLNode>();
		kkPtr<kkXMLNode> subNode = kkCreate(kkXMLNode)();
		kkXMLString name;
		bool next = false;
		while( m_cursor < m_sz ){
			if( m_tokens[ m_cursor ].name == m_expect_lt ){
				if( nextToken() ) return false;
				if( tokenIsName() ){
					name = m_tokens[ m_cursor ].name;
					node->name = name;
					if( nextToken() ) return false;
					// First - attributes
					if( tokenIsName() ){
						--m_cursor;
						if( !getAttributes( node ) ) return false;
					}
					if( m_tokens[ m_cursor ].name == m_expect_gt ){
						if( nextToken() ) return false;
						if( tokenIsName() ){
							node->text = m_tokens[ m_cursor ].name;
							if( nextToken() ) return false;
	closeNode:
							if( m_tokens[ m_cursor ].name == m_expect_lt ){
								if( nextToken() ) return false;
								if( m_tokens[ m_cursor ].name == m_expect_slash ){
									if( nextToken() ) return false;
									if( m_tokens[ m_cursor ].name == name ){
										if( nextToken() ) return false;
										if( m_tokens[ m_cursor ].name == m_expect_gt ){
											++m_cursor;
											return true;
										}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_gt );
									}else return unexpectedToken( m_tokens[ m_cursor ], name );
								}else if( tokenIsName() ){
									--m_cursor;
									subNode = kkCreate(kkXMLNode)();
									goto newNode;
								}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_slash );
							}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_lt );
						}else if( m_tokens[ m_cursor ].name == m_expect_lt ){ // next or </
							if( nextToken() ) return false;
							if( tokenIsName() ){ // next node
								next = true;
								--m_cursor;
							}else if( m_tokens[ m_cursor ].name == m_expect_slash ){ // return true
								if( nextToken() )return false;
								if( m_tokens[ m_cursor ].name == name ){
									if( nextToken() ) return false;
									if( m_tokens[ m_cursor ].name == m_expect_gt ){
										++m_cursor;
										return true;
									}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_gt );
								}else return unexpectedToken( m_tokens[ m_cursor ], name );
							}else return unexpectedToken( m_tokens[ m_cursor ], u"/ or <entity>" );
						}else return unexpectedToken( m_tokens[ m_cursor ], u"\"text\" or <entity>" );
					}else if( m_tokens[ m_cursor ].name == m_expect_slash ){
						if( nextToken() )  return false;
						if( m_tokens[ m_cursor ].name == m_expect_gt ){
							++m_cursor;
							return true;
						}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_gt );
					}else return unexpectedToken( m_tokens[ m_cursor ], u"> or /" );
				}else return unexpectedToken( m_tokens[ m_cursor ], u"name" );
			}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_lt );
			if( next ){
	newNode:
				if( getSubNode( subNode.ptr() ) ){
	///				subNode->addRef();
					node->addNode( subNode.ptr() );
					subNode = nullptr; /// âìåñòî subNode->addRef();
					--m_cursor;
					if( nextToken() ) return false;
					if( m_tokens[ m_cursor ].name == m_expect_lt ){
						if( nextToken() ) return false;
						if( m_tokens[ m_cursor ].name == m_expect_slash ){
							--m_cursor;
							goto closeNode;
						}else if( tokenIsName() ){
							--m_cursor;
							subNode = kkCreate(kkXMLNode)();
							goto newNode;
						}else return unexpectedToken( m_tokens[ m_cursor ], u"</close tag> or <new tag>" );
					}else if( tokenIsName() ){
						node->text = m_tokens[ m_cursor ].name;
						if( nextToken() ) return false;
						if( m_tokens[ m_cursor ].name == m_expect_lt ){
							if( nextToken() )  return false;
							if( m_tokens[ m_cursor ].name == m_expect_slash ){
								--m_cursor;
								goto closeNode;
							}
							else if( tokenIsName() ){
								--m_cursor;
								//subNode.clear();
								subNode = kkCreate(kkXMLNode)();
								goto newNode;
							}else return unexpectedToken( m_tokens[ m_cursor ], u"</close tag> or <new tag>" );
						}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_lt );
					}
				}else return false;
			}
		}
		return true;
	}
	bool getAttributes( kkXMLNode * node ){
		for(;;){
			kkPtr<kkXMLAttribute> at = kkCreate(kkXMLAttribute)();
			if( nextToken() ) return false;
			if( tokenIsName() ){
				at->name = m_tokens[ m_cursor ].name;
				if( nextToken() )  return false;
				if( m_tokens[ m_cursor ].name == m_expect_eq ){
					if( nextToken() )  return false;
					if( m_tokens[ m_cursor ].name == m_expect_apos ) {
						if( nextToken() )  return false;
						//if( tokenIsName() ){
						if( tokenIsString() ){
							at->value = m_tokens[ m_cursor ].name;
							if( nextToken() ) return false;
							if( m_tokens[ m_cursor ].name == m_expect_apos ){
	///							at->addRef();
								node->addAttribute( at.ptr() );
								at = nullptr; /// âìåñòî at->addRef();
								continue;
							}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_apos );
						}
					}else if( m_tokens[ m_cursor ].name == m_expect_quot ){
						if( nextToken() )  return false;
						//if( tokenIsName() ){ //is string
						if( tokenIsString() ){
							at->value = m_tokens[ m_cursor ].name;
							if( nextToken() ) return false;
							if( m_tokens[ m_cursor ].name == m_expect_quot ){
	///							at->addRef();
								node->addAttribute( at.ptr() );
								at = nullptr; /// âìåñòî at->addRef();
								continue;
							}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_quot );
						}
					}else return unexpectedToken( m_tokens[ m_cursor ], u"\' or \"" );
				}else return unexpectedToken( m_tokens[ m_cursor ], m_expect_eq );
			} else if( m_tokens[ m_cursor ].name == m_expect_gt || m_tokens[ m_cursor ].name == m_expect_slash )
				return true;
			else
				return unexpectedToken( m_tokens[ m_cursor ], u"attribute or / or >" );
		} 
		return false;
	}
	bool tokenIsName(){
		if( m_tokens[ m_cursor ].name == m_expect_lt ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_gt ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_sub ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_ex ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_eq ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_apos ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_quot ) return false;
		if( m_tokens[ m_cursor ].name == m_expect_slash ) return false;
		return true;
	}
	bool nextToken(){
		++m_cursor;
		if( m_cursor >= m_sz ){
			fprintf( stderr, "End of XML\n" );
			return true;
		}
		return false;
	}
	bool unexpectedToken( const _token& token, kkXMLString expected ){
		fwprintf( stderr, L"XML: Unexpected token: %s Line:%u Col:%u\n", (const wchar_t*)token.name.data(), token.line, token.col );
		fwprintf( stderr, L"XML: Expected: %s\n", (const wchar_t*)expected.data() );
		return false;
	}
	void skipPrologAndDTD(){
		unsigned int sz = (unsigned int)m_tokens.size();
		while( m_cursor < sz ){
			if( m_tokens[ m_cursor ].name == m_expect_gt ){
				++m_cursor;
				return;
			}else ++m_cursor;
		}
	}
	void printNode( kkXMLNode* node, unsigned int indent ){
		if( node->name.size() ){
			kkXMLString line;
			for( unsigned int i = 0; i < indent; ++i ){
				line += u" ";
			}
			line += u"<";
			line += node->name;
			line += u">";
			if( node->attributeList.size() ){
				line += u" ( ";
				for( unsigned int i = 0; i < node->attributeList.size(); ++i ){
					const kkXMLAttribute * at = node->attributeList[ i ];
					if( at->name.size() ){
						line += at->name;
						line += u":";
						if( at->value.size() ){
							line += u"\"";
							line += at->value;
							line += u"\"";
							line += u" ";
						}else line += u"ERROR ";
					}
				}
				line += u" )";
			}
			if( node->text.size() ){
				line += u" = ";
				line += node->text;
			}
			fwprintf( stdout, L"%s\n", (const wchar_t*)line.data() );
			if( node->nodeList.size() ){
				for( unsigned int i = 0; i < node->nodeList.size(); ++i ){
					printNode( node->nodeList[ i ], ++indent );
					--indent;
				}
			}
		}
	}
	bool tokenIsString(){
		return m_tokens[ m_cursor ].type == kkXMLDocument::_token_type::tt_string;
	}
	bool XPathGetTokens( std::vector<kkXPathToken> * arr, const kkXMLString& XPath_expression ){
		kkXMLString expr = XPath_expression;
		char16_t * ptr = expr.data();
		kkXMLString name;
		char16_t next;
		while( *ptr ){		
			name.clear();
			next = *(ptr + 1);
			kkXPathToken token;
			if( *ptr == u'/' ){
				if( next ){
					if( next == u'/' ){
						++ptr;
						token.m_type = kkXPathTokenType::Double_slash;
					}else token.m_type = kkXPathTokenType::Slash;
				}else token.m_type = kkXPathTokenType::Slash;
			}else if( *ptr == u'*' ){
				token.m_type = kkXPathTokenType::Mul;
			}else if( *ptr == u'=' ){
				token.m_type = kkXPathTokenType::Equal;
			}else if( *ptr == u'\'' ){
				token.m_type = kkXPathTokenType::Apos;
			}else if( *ptr == u'@' ){
				token.m_type = kkXPathTokenType::Attribute;
			}else if( *ptr == u'|' ){
				token.m_type = kkXPathTokenType::Bit_or;
			}else if( *ptr == u',' ){
				token.m_type = kkXPathTokenType::Comma;
			}else if( *ptr == u'+' ){
				token.m_type = kkXPathTokenType::Add;
			}else if( *ptr == u'+' ){
				token.m_type = kkXPathTokenType::Sub;
			}else if( *ptr == u'[' ){
				token.m_type = kkXPathTokenType::Sq_open;
			}else if( *ptr == u']' ){
				token.m_type = kkXPathTokenType::Sq_close;
			}else if( *ptr == u'(' ){
				token.m_type = kkXPathTokenType::Function_open;
			}else if( *ptr == u')' ){
				token.m_type = kkXPathTokenType::Function_close;
			}else if( *ptr == u'<' ){
				if( next ){
					if( next == u'=' ){
						++ptr;
						token.m_type = kkXPathTokenType::Less_eq;
					}else token.m_type = kkXPathTokenType::Less;
				}else token.m_type = kkXPathTokenType::Less;
			}else if( *ptr == u'>' ){
				if( next ){
					if( next == u'/' ){
						++ptr;
						token.m_type = kkXPathTokenType::More_eq;
					}else token.m_type = kkXPathTokenType::More;
				}else token.m_type = kkXPathTokenType::More;
			}else if( *ptr == u':' ){
				if( next ){
					if( next == u':' ){
						++ptr;
						token.m_type = kkXPathTokenType::Axis_namespace;
					}else{
						fprintf( stderr, "XPath: Bad token\n" );
						return false;
					}
				}else{
					fprintf( stderr, "XPath: Bad tokenn" );
					return false;
				}
			}else if( *ptr == u'!' ){
				if( next ){
					if( next == u'=' ){
						++ptr;
						token.m_type = kkXPathTokenType::Not_equal;
					}else{
						fprintf( stderr, "XPath: Bad token\n" );
						return false;
					}
				}else{
					fprintf( stderr, "XPath: Bad token\n" );
					return false;
				}
			}else if( XPathIsName( ptr ) ){
				ptr = XPathGetName( ptr, &name );
				token.m_type = kkXPathTokenType::Name;
				token.m_string = name;
			}else{
				fprintf( stderr, "XPath: Bad token\n" );
				return false;
			}
			arr->push_back( token );
			++ptr;
		}
		return true;
	}
	bool XPathIsName( char16_t * ptr ){
		if( *ptr == u':' ){
			if( *(ptr + 1) == u':' ) return false;
		}
		switch( *ptr ){
		case u'/':
		case u'*':
		case u'\'':
		case u',':
		case u'=':
		case u'+':
		case u'-':
		case u'@':
		case u'[':
		case u']':
		case u'(':
		case u')':
		case u'|':
		case u'!':
			return false;
		}
		return true;
	}
	char16_t* XPathGetName( char16_t*ptr, kkXMLString * name ){
		while( *ptr ){
			if( XPathIsName( ptr ) ) *name += *ptr;
			else break;
			++ptr;
		}
		--ptr;
		return ptr;
	}
	void XPathGetNodes( unsigned int level, unsigned int maxLevel, kkArray<kkXMLString*> elements, kkXMLNode* node, kkArray<kkXMLNode*>* outArr ){
	//_______________________________
		if( node->name == *elements[ level ] ){	
			if( level == maxLevel ){
				outArr->push_back( node );
				return;
			}else{
				unsigned int sz = (unsigned int)node->nodeList.size();
				for( unsigned int  i = 0; i < sz; ++i ){
					XPathGetNodes( level + 1, maxLevel, elements, node->nodeList[ i ], outArr );
				}
			}
		}
	}

	void writeText( kkXMLString& outText, const kkXMLString& inText ){
		u32 sz = inText.size();
		for( u32 i = 0; i < sz; ++i ){
			if( inText[ i ] == u'\'' ){
				outText += u"&apos;";
			}else if( inText[ i ] == u'\"' ){
				outText += u"&quot;";
			}else if( inText[ i ] == u'<' ){
				outText += u"&lt;";
			}else if( inText[ i ] == u'>' ){
				outText += u"&gt;";
			}else if( inText[ i ] == u'&' ){
				outText += u"&amp;";
			}else{
				outText += inText[ i ];
			}
		}
	}
	void writeName( kkXMLString& outText, const kkXMLString& inText ){
		outText += u"<";
		outText += inText;
	}
	bool writeNodes( kkXMLString& outText, kkXMLNode* node, unsigned int tabCount ){
		for( unsigned int i = 0; i < tabCount; ++i )
			outText += u"\t";
		++tabCount;
		writeName( outText, node->name );
		unsigned int sz = (unsigned int)node->attributeList.size();
		if( sz ){
			for( unsigned int i = 0; i < sz; ++i ){
				outText += u" ";
				outText += node->attributeList[ i ]->name;
				outText += u"=";
				outText += u"\"";
				writeText( outText, node->attributeList[ i ]->value );
				outText += u"\"";
			}
		}
		if( !node->nodeList.size() && !node->text.size() ){
			outText += u"/>\r\n";
			return true;
		}else{
			outText += u">\r\n";
			sz = (unsigned int)node->nodeList.size();
			for( unsigned int i = 0; i < sz; ++i ){
				if( !writeNodes( outText, node->nodeList[ i ], tabCount ) ){
					for( unsigned int o = 0; o < tabCount; ++o ){
						outText += u"\t";
					}
					outText += u"</";
					outText += node->nodeList[ i ]->name;
					outText += u">\n";
				}
			}
		}
		if( node->text.size() ){
			for( unsigned int o = 0; o < tabCount; ++o ){
				outText += u"\t";
			}
			writeText( outText, node->text );
			outText += u"\n";
		}
		--tabCount;
		return false;
	}
	bool init(){
		_initExpectStrings();
		if( kkFileExist( m_fileName.data() ) ){
			if( !xmlutil::readTextFromFileForUnicode( m_fileName, m_text ) )
				return false;
		}else
			m_text = m_fileName;
		getTokens();
		if( !analyzeTokens() ) 
			return false;
		m_tokens.clear();
		m_isInit = true;
		return true;
	}
public:
	kkXMLDocument(){}
	~kkXMLDocument(){}
	bool Read( const kkXMLString& file )
	{
		m_fileName = file;
		return init();
	}
	void Write( const kkXMLString& file, bool utf8 ){
		kkXMLString outText( u"<?xml version=\"1.0\"" );
		if( utf8 ) outText += u" encoding=\"UTF-8\"";
		outText += u" ?>\r\n";
		writeNodes( outText, &m_root, 0 );
		outText += u"</";
		outText += m_root.name;
		outText += u">\n";
		auto out = xmlutil::createFileForWriteText( file );
		kkTextFileInfo ti;
		ti.m_hasBOM = true;
		if( utf8 ){
			ti.m_format = kkTextFileFormat::UTF_8;
			out->setTextFileInfo( ti );
			kkXMLStringA mbstr;
			xmlutil::string_UTF16_to_UTF8( outText, mbstr );
			if( ti.m_hasBOM )
				out->write( kkXMLStringA("\xEF\xBB\xBF") );
			out->write( mbstr );
		}else{
			ti.m_endian = kkTextFileEndian::Little;
			ti.m_format = kkTextFileFormat::UTF_16;
			if( ti.m_hasBOM )
				out->write( kkXMLStringA("\xFF\xFE") );
			out->setTextFileInfo( ti );
			out->write( outText );
		}
	}
	kkXMLNode* GetRootNode(){return &m_root;}
	void Print(){
		printf( "XML:\n" );
		printNode( &m_root, 0 );
	}
	const kkXMLString& GetText(){return m_text;}
	kkArray<kkXMLNode*> SelectNodes(const kkXMLString& XPath_expression ){
#ifdef GAME_TOOL
		kkArray<kkXMLNode*> a = kkArray<kkXMLNode*>(0xff);
#else
		kkArray<kkXMLNode*> a;
#endif
		if( !m_isInit ){
			fprintf( stderr, "Bad kkXMLDocument\n" );
			return a;
		}
		std::vector<kkXPathToken> XPathTokens;
		if( !XPathGetTokens( &XPathTokens, XPath_expression ) ){
			fprintf( stderr, "Bad XPath expression\n" );
			return a;
		}
		kkArray<kkXMLString*> elements;
		unsigned int next = 0;
		unsigned int sz = (unsigned int)XPathTokens.size();
		for( unsigned int i = 0; i < sz; ++i ){
			next = i + 1;
			if( i == 0 ){
				if( XPathTokens[ i ].m_type != kkXPathTokenType::Slash && XPathTokens[ i ].m_type != kkXPathTokenType::Double_slash){
					fwprintf( stderr, L"Bad XPath expression \"%s\". Expression must begin with `/`\n", (const wchar_t*)XPath_expression.data() );
					return a;
				}
			}
			switch( XPathTokens[ i ].m_type ){
				case kkXPathTokenType::Slash:
				if( next >= sz ){
					fprintf( stderr, "Bad XPath expression\n" );
					return a;
				}
				if( XPathTokens[ next ].m_type == kkXPathTokenType::Name ){
					elements.push_back( &XPathTokens[ next ].m_string );
					++i;
				}else{
					fwprintf( stderr, L"Bad XPath expression \"%s\". Expected XML element name\n", (const wchar_t*)XPath_expression.data() );
					return a;
				}
				break;
				case kkXPathTokenType::Double_slash:
				break;
				case kkXPathTokenType::Name:
				break;
				case kkXPathTokenType::Equal:
				break;
				case kkXPathTokenType::Not_equal:
				break;
				case kkXPathTokenType::More:
				break;
				case kkXPathTokenType::Less:
				break;
				case kkXPathTokenType::More_eq:
				break;
				case kkXPathTokenType::Less_eq:
				break;
				case kkXPathTokenType::Apos:
				break;
				case kkXPathTokenType::Number:
				break;
				case kkXPathTokenType::Comma:
				break;
				case kkXPathTokenType::Function:
				break;
				case kkXPathTokenType::Function_open:
				break;
				case kkXPathTokenType::Function_close:
				break;
				case kkXPathTokenType::Attribute:
				break;
				case kkXPathTokenType::Bit_or:
				break;
				case kkXPathTokenType::Sq_open:
				break;
				case kkXPathTokenType::Sq_close:
				break;
				case kkXPathTokenType::Div:
				break;
				case kkXPathTokenType::Mod:
				break;
				case kkXPathTokenType::Add:
				break;
				case kkXPathTokenType::Sub:
				break;
				case kkXPathTokenType::Mul:
				break;
				case kkXPathTokenType::And:
				break;
				case kkXPathTokenType::Or:
				break;
				case kkXPathTokenType::Axis_namespace:
				break;
				case kkXPathTokenType::Axis:
				break;
				case kkXPathTokenType::NONE:
				break;
			}
		}
		if( elements.size() ){
			sz = (unsigned int)elements.size();
			XPathGetNodes( 0, sz - 1, elements, &m_root, &a );
		}
		return a;
	}
};

#endif
