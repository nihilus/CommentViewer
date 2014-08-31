static const int widths[] = { 16, 1, 48, 100};
static const char *header[] = {
    "Address",
    "T",
    "Instruction/Data",
    "Comment"
};

//#define _DEBUG_


// last action performed on update
#define MOD_EDIT            0
#define MOD_DELETE          1
#define MOD_UPDATE          2
#define MOD_FAST_UPDATE     3


//index inside the collumns
#define INDEX_ADDRESS		0
#define INDEX_CMT_TYPE      1
#define INDEX_CODE_DATA		2
#define INDEX_COMMENT		3


//options
#define NO_COMMENT          0x00000000

#define INST_NORM_COMMENT	0x00000001
#define INST_REP_COMMENT    0x00000002

#define SHOW_CODE_COMMENT   0x00000004
#define SHOW_DATA_COMMENT   0x00000008
#define PROC_COMMENT        0x00000010
#define SHOW_UNKN_COMMENT   0x00000020
#define SHOW_DFLT_COMMENT   0x00000040
#define SHOW_FUNC_DECLART   0x00000080

// used to show specific icon for type of comment
#define ICON_UNKN_CMT       28
#define ICON_CODE_CMT       74
#define ICON_DATA_CMT       79
#define ICON_PROC_CMT       81
#define ICON_NO_CMT         32

#define OPTIONS_FORM    "Comment Viewer v.0.2\n"\
                        "Options:\n"\
                        "<Show non-repeatable comments:C:1:1::>\n"\
                        "<Show repeatable comment:C:1:1::>>\n"\
                        "<Show code comments:C:1:1::>\n"\
                        "<Show data comments:C:1:1:>\n"\
                        "<Show function comments:C:1:1::>\n"\
                        "<Show undefined bytes comments:C:1:1:>\n"\
                        "<Show IDA's default code comments:C:1:1:>\n"\
                        "<Show function declarations (if exists):C:1:1::>>\n"

//////////////////////////////////////////////////////////////////////////
//	btDestroy
//
//   * Function that is called when list needs update a line
//////////////////////////////////////////////////////////////////////////
ulong idaapi getUpdate(void *obj,ulong n);

//////////////////////////////////////////////////////////////////////////
//	getSize
//
//    * return number of elements in our list (number of comments)
//
//////////////////////////////////////////////////////////////////////////
static ulong idaapi getSize(void *obj);

//////////////////////////////////////////////////////////////////////////
//  GetCommentAddress
//  
//  * Main function of the plugin, it scans the database for comments
// and fills the give node with the addresses which have comments and
// comment type. if // show_wait is set to true, it will show a 
// "please wait" dialog until scanning is over 
//////////////////////////////////////////////////////////////////////////
void GetCommentAddress(void *obj, bool show_wait);
