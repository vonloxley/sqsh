SELECT "char(10)"    = convert(char(10), "test")
SELECT "varchar(10)" = convert(varchar(10), "test")

CREATE TABLE dsp_test (
	char_col    char(10)     null,
	vchar_col   varchar(10)  null,
	bin_col     binary(4)    null,
	text_col    text         null,
	image_col   image        null,
	tiny_col    tinyint      null,
	small_col   smallint     null,
	int_col     int          null,
	real_col    real         null,
	float_col   float        null,
	bit_col     bit          not null,
	dt_col      datetime     null,
	sdt_col     smalldatetime     null,
	money_col   money        null,
	smoney_col  smallmoney   null,
	num_col     numeric(12,3) null,
	dec_col     numeric(12,3) null
)
go

INSERT INTO dsp_test VALUES (
	"test",
	"testtest",
	0x12345678,
	"this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.this is an example of data in a text column, I just need to make sure that the contents of this column exceeds two hundred and fify five characters.",
 0x123456,
 15,
 99,
 23938,
 3.14837,
 99283.1328273,
 1,
 getdate(),
 "11/03/69 04:35am",
 $1237362.32,
 $1233.99,
 3.14837,
 99283.1328273)
go
