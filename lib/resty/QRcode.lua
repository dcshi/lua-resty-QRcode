--module("resty.QRcode", package.seeall)

local setmetatable = setmetatable
local error = error
local ffi = require("ffi")
local HTTP_INTERNAL_SERVER_ERROR = ngx.HTTP_INTERNAL_SERVER_ERROR
local HTTP_OK = ngx.HTTP_OK

module(...)

_VERSION = '0.01'

local mt = { __index = _M } 

--local img_dir = "./img/"

ffi.cdef[[
typedef enum {
	QR_MODE_NUL = -1, 
	QR_MODE_NUM = 0, 
	QR_MODE_AN,        ///< Alphabet-numeric mode
	QR_MODE_8,         ///< 8-bit data mode
	QR_MODE_KANJI,     ///< Kanji (shift-jis) mode
	QR_MODE_STRUCTURE, ///< Internal use only
	QR_MODE_ECI,       ///< ECI mode
	QR_MODE_FNC1FIRST,  ///< FNC1, first position
	QR_MODE_FNC1SECOND, ///< FNC1, second position
} QRencodeMode;

typedef enum {
	QR_ECLEVEL_L = 0, ///< lowest
	QR_ECLEVEL_M,
	QR_ECLEVEL_Q,
	QR_ECLEVEL_H      ///< highest
} QRecLevel;

typedef struct {
	int version;
	int width;
	unsigned char *data;
} QRcode;

typedef struct { 
    int size;                                                                                                                  
    int margin;                                                                                                                
    int dpi;                                                                                                                   
    unsigned int fg_color[4];                                                                                                  
    unsigned int bg_color[4];                                                                                                  
} imgPro;  

QRcode *QRcode_encodeString(const char *string, int version, QRecLevel level, QRencodeMode hint, int casesensitive);

void init(imgPro *pro, int size, int margin, int dpi, const char *fg_val, const char *bg_val);
int save(imgPro *pro, QRcode *qrcode, const char *outfile);
]]

local qr = ffi.load("qrencode")
local img = ffi.load("img")
local img_pro_ptr = ffi.typeof("imgPro[1]")

function new(self, size, margin, dpi, fg_color, bg_color)
	local pro = ffi.new(img_pro_ptr)
	img.init(pro, size, margin, dpi, fg_color, bg_color)

	return setmetatable({ _pro = pro }, mt)
end

function saveQr(self, txt, level, mode, outfile)
	local qrcode = qr.QRcode_encodeString(txt, 0, level, mode, 1)
	local res = img.save(self._pro, qrcode, outfile)

	if res  == -1 then
	    return HTTP_INTERNAL_SERVER_ERROR
	end
	
	return HTTP_OK
end

local class_mt = {
	-- to prevent use of casual module global variables
	__newindex = function (table, key, val)
		error('attempt to write to undeclared variable "' .. key .. '"')
	end
}

setmetatable(_M, class_mt)
