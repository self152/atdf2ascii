/*
@Author : He Yongtao
@Date : 2021/11/16
@Description :
 主要功能是将网站上的所有ODF文件自动解码成为txt文件
  注意一：要提前将网站上的ODF文件和LBL文件下载下来（文件的路径可以更改，但需要与代码中的路劲给保持一致）
  注意二：执行程序时，只需要写上指定文件名而不需要加文件后缀
     如输入命令：./decode GRV_JUGR_2016240_1831XMMMC005V01
         而不是：./decode GRV_JUGR_2016240_1831XMMMC005V01.ODF
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

FILE* fp_lbl;   //读取lbl文件以得到有什么TABLE，以及每个TABLE有多少行
FILE* fp_odf;   //读取odf文件，以后续处理

FILE* output;   //最终输出成一个txt文件
/*整型大小端转换*/
int change_int(int a) {   //针对有符号数的大小端转换函数
    int b = ((a & 0x000000ff) << 24) + ((a & 0x0000ff00) << 8)
        + ((a & 0x00ff0000) >> 8) + ((a & 0xff000000) >> 24);
    return b;
}

/*无符号整型大小端转换*/
unsigned int change_un_int(unsigned int a) {   //针对无符号数的大小端转换函数
    unsigned int b = ((a & 0x000000ff) << 24) + ((a & 0x0000ff00) << 8)
        + ((a & 0x00ff0000) >> 8) + ((a & 0xff000000) >> 24);
    return b;
}

/*所有读取A的结构体*/
struct A_TABLE {
    int primary_key;
    unsigned int second_key;
    unsigned int logical_length;
    unsigned int start_number;
    int zero_1;
    int zero_2;
    int zero_3;
    int zero_4;
    int zero_5;
}a1_table, a2_table, a3_table, a4_14_table, 
    a4_15_table, a4_24_table, a4_25_table, a4_26_table, a4_34_table,
    a4_35_table, a4_36_table, a4_43_table, a4_45_table, a4_54_table, 
    a4_55_table, a4_63_table, a4_65_table, a8_table;

struct ODF1B_TABLE {
    char system_id[8];
    char program_id[8];
    unsigned int spacecraft_id;
    unsigned int file_creation_date;
    unsigned int file_creation_time;
    unsigned int file_reference_date;
    unsigned int file_reference_time;
}b1_table;

struct ODF2B_TABLE {
    char item1[8];
    char item2[8];
    char item3[20];
}b2_table;

struct ODF3C_TABLE {
    unsigned int time_tag;
    int item2_3;//分为10位和22位的数据
    int item4;//OBSERVABLE - INTEGER PART
    int item5;//OBSERVABLE - FRACTIONAL PART
    unsigned int item6_14;//ITEMS 6-14
    int item15_19[2];
    int item20_22[2];//如果用long long 来接这个数：“long long item20_22;”会出现结构体对齐的错误
}c3_table;

/*读取所有的B的结构体*/
struct B_TABLE {
    unsigned int item1;//RAMP START TIME - INTEGER PART
    unsigned int item2;//RAMP START TIME - FRACTIONAL PART
    int item3;//RAMP RATE - INTEGER PART
    int item4;//RAMP RATE - FRACTIONAL PART
    int item5_6;//item5:RAMP START FREQUENCY - GHZ(22位)item6:STATION ID(10位),都需要转成无符号整型
    unsigned int item7;//RAMP START FREQUENCY - INTEGER PART
    unsigned int item8;//RAMP START FREQUENCY - FRACTIONAL PART
    unsigned int item9;//RAMP END TIME - INTEGER PART
    unsigned int item10;//RAMP END TIME - FRACTIONAL PART
}b4_14_table, b4_15_table, b4_24_table, 
    b4_25_table, b4_26_table, b4_34_table, b4_35_table, b4_36_table, 
    b4_43_table, b4_45_table, b4_54_table, b4_55_table, b4_63_table, 
    b4_65_table;

struct ODFB8_TABLE {
    int a[9];
}b8_table;

/*所有写入A table 的函数*/
void writeA(A_TABLE x_table) {
    fprintf(output, "PRIMARY KEY\t");
    fprintf(output, "SECONDARY KEY\t");
    fprintf(output, "LOGICAL RECORD LENGTH\t");
    fprintf(output, "GROUP START PACKET NUMBER\n");
    fprintf(output, "%d\t\t", change_int(x_table.primary_key));
    fprintf(output, "%d\t\t", change_un_int(x_table.second_key));
    fprintf(output, "%d\t\t", change_un_int(x_table.logical_length));
    fprintf(output, "%d\n", change_un_int(x_table.start_number));
}

void write1B(ODF1B_TABLE b1_table) {
    fprintf(output, "SYSTEM ID    PROGRAM ID    SPACECRAFT ID"
            "    FILE CREATION DATE    FILE CREATION TIME    FILE REFERENCE DATE"
            "    FILE REFERENCE TIME\n");
    fprintf(output, "%s\t\t", b1_table.system_id);
    fprintf(output, "%s\t\t", b1_table.program_id);
    fprintf(output, "%d\t\t", change_un_int(b1_table.spacecraft_id));
    fprintf(output, "%d\t\t", change_un_int(b1_table.file_creation_date));
    fprintf(output, "%d\t\t", change_un_int(b1_table.file_creation_time));
    fprintf(output, "%d\t\t", change_un_int(b1_table.file_reference_date));
    fprintf(output, "%4d\t\n", change_un_int(b1_table.file_reference_time));
}

void write2B(ODF2B_TABLE b2_table) {
    fprintf(output, "ITEM1                    ITEM2                   ITEM3\n");
    for (int i = 0; i < 8; i++) {
        fprintf(output, "%c", b2_table.item1[i]);
    }
    fprintf(output, "\t\t");//这里不能用%s来输出，只能一个字符一个字符输出，因为字符和字符串不一样
    for (int i = 0; i < 8; i++) {
        fprintf(output, "%c", b2_table.item2[i]);
    }
    fprintf(output, "\t");
    for (int i = 0; i < 20; i++) {
        fprintf(output, "%c", b2_table.item3[i]);
    }
    fprintf(output, "\n");
}
void write3C(ODF3C_TABLE c3_table) {
    char time_tag[20];
    double a;
    sprintf(time_tag, "%u.%u", change_un_int(c3_table.time_tag), ((unsigned int)(change_int(c3_table.item2_3)) & 0xffc00000 >> 22));
    sscanf(time_tag, "%lf", &a);
    fprintf(output, "%.4lf   ", a);//TIME-TAG                                                               ITEM 1、ITEM 2

    fprintf(output, "%d   ", (unsigned int)(change_int(c3_table.item2_3)) & 0x003fffff);//取后22位                  ITEM 3
    char observable[25];
    double b;

    if (change_int(c3_table.item5) >= 0) {
        double orginal = double(change_int(c3_table.item5)) / 1000000000;
        fprintf(output, "%.10lf  ", double(change_int(c3_table.item4)) + double(change_int(c3_table.item5)) / 1000000000);
        //printf("%.10lf\n", double(change_int(c3_table.item4)) + double(change_int(c3_table.item5)) / 1000000000);
    }
    else {
        double orginal = double(-change_int(c3_table.item5)) / 1000000000;
        fprintf(output, "%.10lf  ", double(change_int(c3_table.item4)) + double(change_int(c3_table.item5)) / 1000000000);
        //printf("%.10lf\n", double(change_int(c3_table.item4)) + double(change_int(c3_table.item5)) / 1000000000);
    }

    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0xe0000000) >> 29);//取3位，FORMAT_ID           ITEM 6
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x1fc00000) >> 22);//取7位，PRI_RECE_STA_ID        ITEM 7
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x003f8000) >> 15);//取7位，TRANS_STATION_ID    ITEM 8
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00006000) >> 13);//取2位，NETWORK_ID          ITEM 9
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00001f80) >> 7);//取6位，DATATYPE _ID         ITEM 10
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00000060) >> 5);//取2位，DOWNLINKBAND_ID      ITEM 11
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00000018) >> 3);//取2位，UPLINKBAND_ID        ITEM 12
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00000006) >> 1);//取2位，EXCITERBAND_ID       ITEM 13
    fprintf(output, "%d     ", ((change_un_int(c3_table.item6_14)) & 0x00000001));//取1位，DATAVALIDITY_INDICATOR    ITEM 14

    long long re_item15_19 = ((long long)(change_int(c3_table.item15_19[0])) << 32) + ((long long)(change_int(c3_table.item15_19[1])));
    fprintf(output, "%d     ", (unsigned int)((re_item15_19 & 0xfe00000000000000) >> 57));//取7位，                 ITEM 15
    fprintf(output, "%d     ", (unsigned int)((re_item15_19 & 0x01ff800000000000) >> 47));//取10位，                    ITEM 16
    fprintf(output, "%d     ", (unsigned int)((re_item15_19 & 0x0000400000000000) >> 46));//取1位，                 ITEM 17

    char frequence[24];
    double c;
    sprintf(frequence, "%u.%u", ((unsigned int)((re_item15_19 & 0x00003fffff000000) >> 24)), ((unsigned int)(re_item15_19 & 0x0000000000ffffff)));
    sscanf(frequence, "%lf", &c);
    fprintf(output, "%.8lf   ", c);//FREQUENCE-REFERENCE                                                       ITEM 18、ITEM 19

    long long re_item20_22 = ((long long)(change_int(c3_table.item20_22[0])) << 32) + ((long long)(change_int(c3_table.item20_22[1])));
    fprintf(output, "%d     ", (unsigned int)((re_item20_22 & 0xfffff00000000000) >> 44));//取20位，                    ITEM 20
    fprintf(output, "%d     ", (unsigned int)((re_item20_22 & 0x00000fffffc00000) >> 22));//取22位,                 ITEM 21
    fprintf(output, "%d\n", (unsigned int)(re_item20_22 & 0x00000000003fffff));//取后22位，                         ITEM 22
}

/*所有写入B table 的函数*/
void writeB(B_TABLE x_table) {
    char ramp_start_time[24];
    double a;
    sprintf(ramp_start_time, "%u.%u", change_un_int(x_table.item1), change_un_int(x_table.item2));
    sscanf(ramp_start_time, "%lf", &a);
    fprintf(output, "%.4lf   ", a);//RAMP START TIME         1、2合并
    char ramp_rate[24];
    double b;
    /*需要判断第二个数是整数还是负数，负数拼接的时候需要去掉负号*/
    if (change_int(x_table.item4) >= 0) {
        sprintf(ramp_rate, "%d.%d", change_int(x_table.item3), change_int(x_table.item4));
        sscanf(ramp_rate, "%lf", &b);
        fprintf(output, "%.10lf   ", b);//RAMP RATE             原始的item3、4合并
    }
    else {
        sprintf(ramp_rate, "%d.%d", change_int(x_table.item3), (-change_int(x_table.item4)));
        sscanf(ramp_rate, "%lf", &b);
        fprintf(output, "%.10lf   ", b);//RAMP RATE             原始的item3、4合并
    }
    fprintf(output, "%d   ", (unsigned int)(change_int(x_table.item5_6) & 0xfffffc00 >> 10));//取前22位,原始的item5
    fprintf(output, "%d   ", (unsigned int)(change_int(x_table.item5_6) & 0x000003ff));//取前22位,原始的item6

    char ramp_start_frequence[24];
    double c;
    sprintf(ramp_start_frequence, "%u.%u", change_un_int(x_table.item7), change_un_int(x_table.item8));
    sscanf(ramp_start_frequence, "%lf", &c);
    fprintf(output, "%.10lf   ", c);//RAMP START FREQUENCE      原始的item7、8合并

    char ramp_end_time[24];
    double d;
    sprintf(ramp_end_time, "%u.%u", change_un_int(x_table.item9), change_un_int(x_table.item10));
    sscanf(ramp_end_time, "%lf", &d);
    fprintf(output, "%.10lf\n", d);//RAMP END TIME             原始的item9、10合并
}

void write8B_table(ODFB8_TABLE b8_table) {
    for (int i = 0; i < 9; i++) {
        fprintf(output, "%d\t", change_int(b8_table.a[i]));
    }
    fprintf(output, "\n");
}

/*写所有的A_TABLE*/
void writeA_data(A_TABLE x_table) {
    fread(&x_table, sizeof(x_table), 1, fp_odf);//读取
    writeA(x_table);//写入头部ODF1A_TABLE
    fprintf(output, "-----------------------------------------------------------------"
            "-----------------------------------\n");//分割线
}

/*写所有的B_TABLE*/
void writeB_data(B_TABLE x_table, int n) {
    for (int i = 0; i < n; i++) {
        fread(&x_table, sizeof(x_table), 1/*读取1个数据项*/, fp_odf);
        writeB(x_table);//写入ODF1B_TABLE
    }
    fprintf(output, "-----------------------------------------------------------------"
            "-----------------------------------\n");//分割线
}

/*所有的B的title*/
void B_all_title() {
    fprintf(output, "Item 1:RAMP START TIME                           Item 2:RAMP RATE\n"
            "Item 3:RAMP START FREQUENCY - GHZ     Item 4:STATION ID\n"
            "Item 5:RAMP START FREQUENCY               Item 6:RAMP END TIME\n");
    fprintf(output, "        Item 1\t Item 2        Item 3  Item 4\t     Item 5\t\t    Item 6\n");
}

void decode(vector<string> tables, vector<int> lines) {
    for (int i = 0; i < tables.size(); i++) {
        int tem = lines[i];
        /*从1A到3C，开始*/
        if (tables[i] == "ODF1A_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF1A_TABLE:FILE LABEL GROUP HEADER");//写ODF1A_TABLE title
            writeA_data(a1_table);
            printf("ODF1A_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);

        }
        else if (tables[i] == "ODF1B_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF1B_TABLE:FILE LABEL GROUP DATA");
            fread(&b1_table, sizeof(b1_table), 1/*读取1个数据项*/, fp_odf);
            write1B(b1_table);//写入ODF1B_TABLE
            fprintf(output, "---------------------------------------------"
                    "-------------------------------------------------------\n");//分割线
            printf("ODF1B_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF2A_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF2A_TABLE:IDENTIFIER GROUP HEADER");//写ODF2A_TABLE title
            writeA_data(a2_table);
            printf("ODF2A_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF2B_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF2B_TABLE:IDENTIFIER GROUP DATA");
            fread(&b2_table, sizeof(b2_table), 1/*读取1个数据项*/, fp_odf);
            write2B(b2_table);//写入ODF2B_TABLE
            fprintf(output, "-----------------------------------------------------------------"
                    "-----------------------------------\n");//分割线
            printf("ODF2B_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF3A_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF3A_TABLE:ORBIT DATA GROUP HEADER");//写ODF3A_TABLE title
            writeA_data(a3_table);
            printf("ODF3A_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF3C_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF3C_TABLE:ORBIT DATA GROUP DATA");//写ODF3C_TABLE title
            fprintf(output, "Item 1 : TIME TAG\n\tDESCRIPTION :  The record time tag, measured from 0 "
                    "hours UTC on 1 January 1950.\nItem 2 : PRIMARY RECEIVING STATION DOWNLINK DELAY\n\t"
                    "DESCRIPTION :  Downlink delay for the primary receiving station.\n"
                    "Item 3 : OBSERVABLE\nItem 4 : FORMAT ID\n\tDESCRIPTION :  The Format ID.Set to 2.If this"
                    "value is 1, the ODF was created on or before 1997-04-14\n\tand will not be accurately"
                    "described by this set of object definitions.If FORMAT ID = 1, see :\n\tJPL / DSN Document 820 - 13\n"
                    "Item 5 : PRIMARY RECEIVING STATION ID\n\tDESCRIPTION :  The ID Number of the primary "
                    "Receiving Station.\nItem 6 : TRANSMITTING STATION ID\n\tDESCRIPTION :Transmitting Station ID"
                    "Number.Set to zero if quasar VLBI, one - way (Dippler, phase, or\n\trange), or angles data.\n"
                    "Item 7 : NETWORK ID\n\tDESCRIPTION :  Network ID Number for primary Receiving  Station : "
                    "Set to : \n\t0   for DSN, Block V exciter\n\t1   for other\n\t2   for OTS(OVLBI Tracking Subnet,"
                    "where OVLBI is Orbiting VLBI)\nItem 8 : DATA TYPE ID\n\t"
                    "DESCRIPTION :  Data Type ID Number.\n\t"
                    "Allowed data type values include :\n\t"
                    "01 = Narrowband spacecraft VLBI, Doppler mode; cycles\n\t"
                    "02 = Narrowband spacecraft VLBI, phase mode; cycles\n\t"
                    "03 = Narrowband quasar VLBI, Doppler mode; cycles\n\t"
                    "04 = Narrowband quasar VLBI, phase mode; cycles\n\t"
                    "05 = Wideband spacecraft VLBI; nanoseconds\n\t"
                    "06 = Wideband quasar VLBI; nanoseconds\n\t"
                    "11 = One - way Doppler; Hertz\n\t"
                    "12 = Two - way Doppler; Hertz\n\t"
                    "13 = Three - way Doppler; Hertz\n\t"
                    "21 = One - way total - count phase; cycles\n\t"
                    "22 = Two - way total - count phase; cycles\n\t"
                    "23 = Three - way total - count phase; cycles\n\t"
                    "36 = PRA Planetary operational discrete spectrum range;range units\n\t"
                    "37 = SRA Planetary operational discrete spectrum range;range units\n\t"
                    "41 = RE[GSTDN] Range; nanoseconds\n\t"
                    "51 = Azimuth angle; degrees\n\t"
                    "52 = Elevation angle; degrees\n\t"
                    "53 = Hour angle; degrees\n\t"
                    "54 = Declination angle; degrees\n\t"
                    "55 = X angle(where + X is east); degrees\n\t"
                    "56 = Y angle(where + X is east); degrees\n\t"
                    "57 = X angle(where + X is south); degrees\n\t"
                    "58 = Y angle(where + X is south); degrees\n"
                    "Item 9 : DOWNLINK BAND ID\n\tDESCRIPTION :Downlink Band ID.Allowed"
                    "values include : \n\t0 = Not applicable if angle data,\n\t"
                    "Ku - band otherwise\n\t"
                    "1 = S - band\n\t"
                    "2 = X - band\n\t"
                    "3 = Ka - band\n"
                    "Item 10 : UPLINK BAND ID\n\t"
                    "DESCRIPTION :  Uplink Band ID.Allowed values include :\n\t"
                    "0 = Not applicable if angle data or 1 - way data,\n\t"
                    "Ku - band otherwise\n\t"
                    "1 = S - band\n\t"
                    "2 = X - band\n\t"
                    "3 = Ka - band\n"
                    "Item 11 : EXCITER BAND ID\n\tDESCRIPTION : Exciter Band ID.Allowed"
                    "values include :\n\t"
                    "0 = Not applicable if angle data,\n\t"
                    "Ku - band otherwise\n\t"
                    "1 = S - band\n\t"
                    "2 = X - band\n\t"
                    "3 = Ka - band\n"
                    "Item 12 : DATA VALIDITY INDICATOR\n\t"
                    "DESCRIPTION :  The data validity flag.Values are :\n\t"
                    "0 = good\n\t"
                    "1 = bad\n"
                    "Item 13 : SECOND RECEIVING STATION ID\n\t"
                    "DESCRIPTION : Second receiving station ID number, if VLBI data; Lowest(last) "
                    "component, if PRA / SRA\n\trange data; Integer seconds of observable, if RE range data;"
                    "Set to  0, otherwise.\n"
                    "Item 14 : Quasar ID\n\t"
                    "DESCRIPTION : Quasar ID, if VLBI quasar data; Spaceraft ID, otherwise.\n"
                    "Item 15 : MODULUS INDICATOR\n\t"
                    "DESCRIPTION : Modulus indicator, if wideband VLBI data; Phase Point indicator,"
                    "if narrowband VLBI data;\n\tReceiver / exciter independent flag, if Doppler,"
                    "phase, or range data(0 = no, 1 = yes); Set to  0, otherwise.\n"
                    "Item 16 : REFERENCE FREQUENCE\n\t"
                    "DESCRIPTION : Reference frequency, milliHertz :Transponder frequency, if one - way Doppler"
                    "or phase;\n\tReceiver frequency, if ramped and not one-way; Transmitter frequency otherwise;"
                    "Set to 0, if angles data.\n"
                    "Item 17 :\n\t"
                    "DESCRIPTION : If narrowband VLBI data :\n\t"
                    "(Phase Calibration Flag minus 1) times 100000, plus Channel ID Number times 10000.\n\t"
                    "If wideband VLBI data : \n\t"
                    "(Channel Sampling Flag minus 1) times 100000, plus Mode ID number times 10000, plus"
                    "Modulus high-part\n\tin 10^-1 nanoseconds.\n\t"
                    "If OTS Doppler data : \n\tTrain Axis Angle in millidegrees.\n\t"
                    "If PRA / SRA range data : \n\tUplink Ranging Transmitter In - Phase Time Offset from"
                    "Sample Timetag in seconds,otherwise, set to 0.\n"
                    "Item 18 :\n\t"
                    "DESCRIPTION : If wideband VLBI data : \n\t"
                    "low - part(units are nanoseconds after the value is multiplied by 10 ^ -7).\n\t"
                    "If Doppler, phase, or narrowband VLBI data :Compression time in hundredths of a second.\n\t"
                    "If PRA / SRA range data :\n\tHighest(first) Component times 100000, plus Downlink Ranging "
                    "Transmitter Coder In-Phase\n\tTime Offset from Sample Timetag in seconds.Otherwise,set to 0.\n"
                    "Item 19 :\n\t"
                    "DESCRIPTION : If VLBI data : \n\t"
                    "Second Receiving Station Downlink Delay in nanoseconds.\n\t"
                    "If Doppler, phase, or range data :\n\t"
                    "Transmitting Station Uplink Delay in nanoseconds.Otherwise, set to 0.\n");
            fprintf(output, "         I-1               I-2         I-3               I-4    I-5    I-6    I-7   I-8   "
                    "I-9  I-10  I-11 I-12 I-13 I-14 I-15               I-16         I-17  I-18    I-19\n");
            //printf("%d\n", sizeof(long long));
            for (int j = 0; j < lines[i]; j++) {
                fread(&c3_table, sizeof(c3_table), 1, fp_odf);//读取
                write3C(c3_table);//写入头部ODF3C_TABLE
            }
            fprintf(output, "-----------------------------------------------------------------"
                    "-----------------------------------\n");//分割线
            printf("ODF3C_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*从1A到3C，结束*/

        /*4A和4B的14、15，开始*/
        else if (tables[i] == "ODF4A14_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A14_TABLE:RAMP GROUP 14 HEADER");//写ODF4A14_TABLE title
            writeA_data(a4_14_table);
            printf("ODF4A14_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B14_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B14_TABLE:RAMP GROUP 14 DATA");//写ODF4B14_TABLE title
            B_all_title();
            writeB_data(b4_14_table, lines[i]);
            printf("ODF4B14_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A15_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A15_TABLE:RAMP GROUP 15 HEADER");//写ODF4A14_TABLE title
            writeA_data(a4_15_table);
            printf("ODF4A15_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B15_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B15_TABLE:RAMP GROUP 15 DATA");//写ODF4B14_TABLE title
            B_all_title();
            writeB_data(b4_15_table, lines[i]);
            printf("ODF4B15_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的14、15，结束*/

        /*4A和4B的24、25、26，开始*/
        else if (tables[i] == "ODF4A24_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A24_TABLE:RAMP GROUP 24 HEADER");//写ODF4A14_TABLE title
            writeA_data(a4_24_table);
            printf("ODF4A24_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B24_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B24_TABLE:RAMP GROUP 24 DATA");//写ODF4B24_TABLE title
            B_all_title();
            writeB_data(b4_24_table, lines[i]);
            printf("ODF4B24_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A25_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A25_TABLE:RAMP GROUP 25 HEADER");//写ODF4A25_TABLE title
            writeA_data(a4_25_table);
            printf("ODF4A25_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B25_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B25_TABLE:RAMP GROUP 25 DATA");//写ODF4B25_TABLE title
            B_all_title();
            writeB_data(b4_25_table, lines[i]);
            printf("ODF4B25_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A26_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A26_TABLE:RAMP GROUP 26 HEADER");//写ODF4A26_TABLE title
            writeA_data(a4_26_table);
            printf("ODF4A26_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B26_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B26_TABLE:RAMP GROUP 26 DATA");//写ODF4B26_TABLE title
            B_all_title();
            writeB_data(b4_26_table, lines[i]);
            printf("ODF4B26_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的24、25、26，结束*/

        /*4A和4B的34、35、36，开始*/
        else if (tables[i] == "ODF4A34_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A34_TABLE:RAMP GROUP 34 HEADER");//写ODF4A34_TABLE title
            writeA_data(a4_34_table);
            printf("ODF4A34_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B34_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B34_TABLE:RAMP GROUP 34 DATA");//写ODF4B34_TABLE title
            B_all_title();
            writeB_data(b4_34_table, lines[i]);
            printf("ODF4B34_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A35_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A35_TABLE:RAMP GROUP 35 HEADER");//写ODF4A35_TABLE title
            writeA_data(a4_35_table);
            printf("ODF4A35_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B35_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B35_TABLE:RAMP GROUP 35 DATA");//写ODF4B35_TABLE title
            B_all_title();
            writeB_data(b4_35_table, lines[i]);
            printf("ODF4B35_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A36_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A36_TABLE:RAMP GROUP 36 HEADER");//写ODF4A36_TABLE title
            writeA_data(a4_36_table);
            printf("ODF4A36_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B36_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B36_TABLE:RAMP GROUP 36 DATA");//写ODF4B36_TABLE title
            B_all_title();
            writeB_data(b4_36_table, lines[i]);
            printf("ODF4B36_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的34、35、36，结束*/

        /*4A和4B的43、45，开始*/
        else if (tables[i] == "ODF4A43_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A43_TABLE:RAMP GROUP 43 HEADER");//写ODF4A43_TABLE title
            writeA_data(a4_43_table);
            printf("ODF4A43_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B43_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B43_TABLE:RAMP GROUP 43 DATA");//写ODF4B43_TABLE title
            B_all_title();
            writeB_data(b4_43_table, lines[i]);
            printf("ODF4B43_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }

        else if (tables[i] == "ODF4A45_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A45_TABLE:RAMP GROUP 45 HEADER");//写ODF4A45_TABLE title
            writeA_data(a4_45_table);
            printf("ODF4A45_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B45_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B45_TABLE:RAMP GROUP 45 DATA");//写ODF4B45_TABLE title
            B_all_title();
            writeB_data(b4_45_table, lines[i]);
            printf("ODF4B45_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的43、45，结束*/

        /*4A和4B的54、55，开始*/
        else if (tables[i] == "ODF4A54_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A54_TABLE:RAMP GROUP 54 HEADER");//写ODF4A54_TABLE title
            writeA_data(a4_54_table);
            printf("ODF4A54_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B54_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B54_TABLE:RAMP GROUP 54 DATA");//写ODF4B54_TABLE title
            B_all_title();
            writeB_data(b4_54_table, lines[i]);
            printf("ODF4B54_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A55_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A55_TABLE:RAMP GROUP 55 HEADER");//写ODF4A55_TABLE title
            writeA_data(a4_55_table);
            printf("ODF4A55_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B55_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B55_TABLE:RAMP GROUP 55 DATA");//写ODF4B55_TABLE title
            B_all_title();
            writeB_data(b4_55_table, lines[i]);
            printf("ODF4B55_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的54、55，结束*/

        /*4A和4B的63、65，开始*/
        else if (tables[i] == "ODF4A63_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A63_TABLE:RAMP GROUP 63 HEADER");//写ODF4A63_TABLE title
            writeA_data(a4_63_table);
            printf("ODF4A63_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B63_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B63_TABLE:RAMP GROUP 63 DATA");//写ODF4B63_TABLE title
            B_all_title();
            writeB_data(b4_63_table, lines[i]);
            printf("ODF4B63_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4A65_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4A65_TABLE:RAMP GROUP 65 HEADER");//写ODF4A65_TABLE title
            writeA_data(a4_65_table);
            printf("ODF4A65_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF4B65_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF4B65_TABLE:RAMP GROUP 65 DATA");//写ODF4B65_TABLE title
            B_all_title();
            writeB_data(b4_65_table, lines[i]);
            printf("ODF4B65_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*4A和4B的63、65，结束*/

        /*8A和8B，开始*/
        else if (tables[i] == "ODF8A_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF8A_TABLE:END OF FILE GROUP HEADER");//写ODF8A_TABLE title
            writeA_data(a8_table);
            printf("ODF8A_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        else if (tables[i] == "ODF8B_TABLE") {
            fprintf(output, "\t\t%s\n", "ODF8B_TABLE:END OF FILE GROUP DATA");//写ODFB8_TABLE title
            fprintf(output, "SPARE\n");
            for (int j = 0; j < lines[i]; j++) {
                fread(&b8_table, sizeof(b8_table), 1, fp_odf);//读取
                write8B_table(b8_table);//写入头部ODFB8_TABLE
            }
            fprintf(output, "---------------------------------------------"
                    "-------------------------------------------------------\n");//分割线
            printf("ODF8B_TABLE\t");
            printf("i = %d, lines[i] = %d\n", i, lines[i]);
        }
        /*8A和8B，结束*/
        else return;
    }
}


int main(int argc, char* argv[]) {
    /****************处理输入，开始***************/
    /*for (int i = 0; i < argc; i++) {
        cout << argv[i] << endl;
    }
    */
    string strArr = argv[1];
    string routeOfLBL = "/home/hyt/decoding/lblAll/" + strArr;
    routeOfLBL += ".LBL";

    string routeOfODF = "/home/hyt/decoding/odfAll/" + strArr;
    routeOfODF += ".ODF";

    string routeOfTXT = "/home/hyt/decoding/txtAll/" + strArr;
    routeOfTXT += ".TXT";
    int strLBLSize = routeOfLBL.size();
    char LBLtem[strLBLSize];
    for (int i = 0; i < routeOfLBL.size(); i++) {
        LBLtem[i] = routeOfLBL[i];
    }

    int strODFSize = routeOfODF.size();
    char ODFtem[strODFSize];
    for (int i = 0; i < routeOfODF.size(); i++) {
        ODFtem[i] = routeOfODF[i];
    }

    int strTXTSize = routeOfTXT.size();
    char TXTtem[strTXTSize];
    for (int i = 0; i < routeOfTXT.size(); i++) {
        TXTtem[i] = routeOfTXT[i];
    }
    /****************处理输入，结束***************/


    /************读取lbl文件，最终得到所有table和各个table下的数据行数。开始***************/
    fp_lbl = fopen(LBLtem, "r");
    if (fp_lbl == NULL) {
        cout << "Fail to open LBL file!" << endl;
        return 0;
    }

    int lblLineIndex;  //代表是第几行
    vector<string> allTable;//拿到所以的table
    vector<int> allLines;   //拿到所有的行数，原始行数，没有相减
    char linesSum[9];       //第四行的数，代表解码出来的文件总数据行数（暂时存在这里）
    int linesAll = 0;       //转成一个整型作为原始行数的最后一个元素
    int linesSumIndex = 0;
    char everyTable[14];
    char everyLine[6];
    while (feof(fp_lbl) == 0) {
        char a[256];
        for (lblLineIndex = 0; lblLineIndex < 3; lblLineIndex++) {
            fgets(a, 256, fp_lbl);
        }

        fgets(a, 256, fp_lbl);     //第四行的内容
        int j = 30;
        while (j < 38) {
            linesSum[linesSumIndex] = a[j];
            j++; linesSumIndex++;
        }
        linesAll = atoi(linesSum);

        for (lblLineIndex = 4; lblLineIndex < 38; lblLineIndex++) {
            fgets(a, 256, fp_lbl);
        }

        for (lblLineIndex = 39; lblLineIndex < 120; lblLineIndex++) {
            fgets(a, 256, fp_lbl);
            int tableIndex = 0;
            int linesIndex = 0;
            int charIndex = 0;    //each line(a[256])'s character index
            if (a[0] == '^') {    //the lines we want to find
                string table = "";
                charIndex++;
                while (a[charIndex] != ' ') {
                    table += a[charIndex++];//put together
                }

                /////*string table = (string)everyTable;  //chars to string*/////
                /////use (string) can be wrong! Be aware!////
                allTable.push_back(table);
                while (a[charIndex] != ',') {
                    charIndex++;
                }
                charIndex++;
                while (a[charIndex] != ')') {   //characters between ',' and )
                    everyLine[linesIndex] = a[charIndex];//put together
                    charIndex++;
                    linesIndex++;
                }
                allLines.push_back(atof(everyLine));
            }
        }
        break;
    }
    allLines.push_back(linesAll);
    fclose(fp_lbl);


    vector<int> finalAllLines(allTable.size());
    for (int i = 0; i < allLines.size() - 1; i++) {
        finalAllLines[i] = allLines[i + 1] - allLines[i];
    }

    //到这里，得到了每个TABLE，以及每个TABLE对应的行数
    /************读取lbl文件，最终得到所有table和各个table下的数据行数。结束***************/

    /************有用的两个数组为：allTable和finalAllLines*************/
    fp_odf = fopen(ODFtem, "r");
    if (fp_odf == NULL) {
        printf("Fail to open ODF file!");
        return -1;
    }
    /*处理ODF文件的代码，开始*/
    output = fopen(TXTtem, "w");

    decode(allTable, finalAllLines);

    /*处理ODF文件的代码，结束*/
    fclose(fp_odf);
    fclose(output);
    return 0;
}
