libqrencode(http://fukuchi.org/works/qrencode/index.html.en) and libpng is need:
sudo apt-get install libqrencode-dev libpng12-dev

How to Use

local resty_qr = require "resty.QRcode";

local str = "http://dcshi.com";

local file = "/tmp/qr.png";

local color = "FF00F3";  --RGB

local qr = resty_qr:new(8, 4, 72, color);  -- size margin dpi fg_color bg_color

local status = qr:saveQr(str, 0, 2, file)    -- str level mode path_img
