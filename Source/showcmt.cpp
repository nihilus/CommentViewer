// ***************************************************************
//  ShowComment:  0.2   ·  date: 09/25/2006
//  -------------------------------------------------------------
//  A simple plugin to ease navigation of comments inside the
//  disassembly.
//  History:
//      10  oct - Testing period is over. Lots of minor bugs fixed.
//      01  oct - Added support for multiple windows and per
//              segment view of comments. 
//      29  sept - Fixed minor bugs, added option to edit comments
//              and also improved speed by tweaking the update
//              function
//      28  sept - First version over. Testing time.
//      25  sept - Started coding
//  -------------------------------------------------------------
//  Copyright (C) bLaCk-eye/RET - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <bytes.hpp>
#include <kernwin.hpp>
#include <name.hpp>
#include <auto.hpp>
#include <typeinf.hpp>
#include <struct.hpp>
#include <gdl.hpp>
#include "showcmt.h"

unsigned short winCount = 0 ;//number of opened windows
short opt1= INST_REP_COMMENT ;  //initial state of options
short opt2 = (SHOW_CODE_COMMENT |
            SHOW_DATA_COMMENT |          
            PROC_COMMENT |
            SHOW_FUNC_DECLART) >> 2 ;

bool isInited = false;

void ShowBanner()
{
    msg(" *********************************************\n");
    msg(" *** CommentViewer v.0.2 (c) bLaCk-eye/RET ***\n");
    msg(" *** Bugs, suggestions : bLaCk@reteam.org  ***\n");
    msg(" *********************************************\n");
}


//////////////////////////////////////////////////////////////////////////
//  initPlugin
//
//  - initializes the plugin when first time loaded by Ida
//////////////////////////////////////////////////////////////////////////
int initPlugin(void)
{

    return PLUGIN_OK;
}

//////////////////////////////////////////////////////////////////////////
//  termPlugin
//
//  - called by Ida when terminating the plugin
//  seems its usually empty so we leave it empty too
//////////////////////////////////////////////////////////////////////////
void termPlugin(void) {
}


//////////////////////////////////////////////////////////////////////////
//  GetCommentType
//
//  return values
//		- NO_COMMENT     - no comment found at given address
//		- INST_COMMENT   - repeatable instruction comment
//      - PROC_COMMENT   - repeatable procedure comment
//
//////////////////////////////////////////////////////////////////////////
int GetCommentType(ea_t ea)
{

    flags_t tflag = getFlags(ea);
    func_t *fct = getn_func(get_func_num(ea));
    
    
    //we scan first for procedure comments
    //because if there is a non repeatable comment at first line
    //of the procedure then Ida doesn't set it as comment for the 
    //proc so it will appear as a normal comment instead 
    //of a proc comment
    // to be quick: fct comments have priority

    if (fct != NULL &&						// we got a procedure?
        fct->startEA ==ea &&				// only show first instruction?
        get_func_cmt(fct, true)!=NULL)		// it has comment?
        
        return PROC_COMMENT;
    
    if (has_cmt(tflag) && get_cmt(ea, false, NULL, MAXSTR) != -1)
        return INST_NORM_COMMENT;
    
    if (has_cmt(tflag) && get_cmt(ea, true, NULL, MAXSTR) != -1)
        return INST_REP_COMMENT;

   
    return NO_COMMENT;
}


//////////////////////////////////////////////////////////////////////////
//  GetComment
//  
//  * Function which fill ptcomment with the comment found at the given
//  address. It checks automatically the type of comment and removes newlines
//  for better viewing
//////////////////////////////////////////////////////////////////////////
void GetComment(ea_t ea, char* ptcomment, bool rmv_crfl)
{
    char* origcmt;
    
    int cmttype = GetCommentType(ea);
    
    if (cmttype == NO_COMMENT)
    { 
        ptcomment[0] = '\0';
        return;
    }
    else
        if (cmttype == INST_NORM_COMMENT)
            get_cmt(ea, false, ptcomment, MAXSTR);
        else
            if (cmttype == INST_REP_COMMENT)
                get_cmt(ea, true, ptcomment, MAXSTR);
        else
        {
            origcmt = get_func_cmt(getn_func(get_func_num(ea)), true);
            qstrncpy(ptcomment, origcmt, MAXSTR);
            qfree(origcmt);
        }
    
    //remove CR/LF   
     rmv_crfl ? strrpl(ptcomment, 0xa, ' ') : 0;
}

void SetComment(ea_t ea, char* ptcomment)
{
    
    switch(GetCommentType(ea))
    {

    case NO_COMMENT:
        break;

    case INST_NORM_COMMENT:
        set_cmt(ea, ptcomment, false);
        break;

    case INST_REP_COMMENT:
        set_cmt(ea, ptcomment, true);
        break;

    case PROC_COMMENT:
        set_func_cmt(getn_func(get_func_num(ea)),ptcomment, true);
        break;
    }    
}

//////////////////////////////////////////////////////////////////////////
//  IsComment
//
//  *  Callback function called by nextthat function. Used to select
// regions that have comments and are head of data/code.
//////////////////////////////////////////////////////////////////////////
bool idaapi IsComment(flags_t flags, void *ud)
{
    return (has_cmt(flags) || isFunc(flags));
}


//////////////////////////////////////////////////////////////////////////
//  GetCommentAddress
//  
//  * Main function of the plugin, it scans the database for comments
// and fills the give node with the addresses which have comments and
// comment type. if // show_wait is set to true, it will show a 
// "please wait" dialog until scanning is over 
//////////////////////////////////////////////////////////////////////////
void GetCommentAddress(void *obj, bool show_wait)
{
    char cmt[MAXSTR];
    long cmttype;
    int counter = 0;	
    ea_t curr;
    idastate_t oldstate;

    netnode *node = (netnode *)obj;
    
    if (show_wait)
	{
		oldstate = setStat(st_Work);
		show_wait_box("Scanning database for comments...");
		
	}
    
    // start and end address of area to scan for comments
    ea_t start_ea = node->altval(-2);
    ea_t end_ea   = node->altval(-3);

    //get options
    short options = node->altval(-4);
    
    
    curr = nextthat(start_ea-1,end_ea, IsComment, NULL);
    while (curr != BADADDR)
    {	
        cmttype = GetCommentType(curr);
        
        flags_t fl = getFlags(curr);

        bool show_code = (isCode(fl) && (options & SHOW_CODE_COMMENT));
        bool show_data = (isData(fl) && (options & SHOW_DATA_COMMENT));
        bool show_unkn = (isUnknown(fl) && (options & SHOW_UNKN_COMMENT));
        bool show_proc = (cmttype == PROC_COMMENT) && (options & INST_REP_COMMENT);
        bool show_dflt = is_tilcmt(curr) && (options & SHOW_DFLT_COMMENT);
        
        if (cmttype != NO_COMMENT &&       // if there is a comment
            ((cmttype & options)!= 0) &&   // and comment type is valid 
            (show_code || show_data || show_unkn)) //and we should show it
        {
            //don't show comments if INST_REP_COMMENT not enable
            if (!(cmttype == PROC_COMMENT && show_proc ==false) && 
                !(is_tilcmt(curr) && show_dflt == false))    
            {
                //set address 
                node->altset(counter, curr);
            
                //set comment
                GetComment(curr, (char*) cmt, true);
                node->supset(counter++, &cmt, sizeof(cmt));
            }
        }
        
        curr = nextthat(curr, end_ea, IsComment, NULL);			
    }

    //number of comments
    node->altset(-1, counter);
   
    if (show_wait)
	{
		setStat(oldstate);
		hide_wait_box();
	}
    
}

//////////////////////////////////////////////////////////////////////////
//	getSize
//
//    * return number of elements in our list (number of comments)
//
//////////////////////////////////////////////////////////////////////////
static ulong idaapi getSize(void *obj)
{
    //msg("size()!\n");
    netnode *node = (netnode *)obj;
    return node->altval(-1);       // we have saved the number in altval(-1)
}


//////////////////////////////////////////////////////////////////////////
//	getDescription
//
//	 * Generates description line for the listing
//////////////////////////////////////////////////////////////////////////
#define VAR_LEN 64	//name of the var is 31 chars max
static void idaapi getDescription(void *obj,ulong n,char * const *arrptr)
{
    netnode *node = (netnode *)obj;   
    //msg("getDescription(%.8x)\n", n);
    // generate the column headers
    if ( n == 0 )
    {
        for ( int i=0; i < qnumber(header); i++ )
            qstrncpy(arrptr[i], header[i], MAXSTR);
        return;
    }
        
    //any entries in the list?
    if (!node->altval(-1))
        return;
    
    // get address 
    ea_t ea = node->altval(n-1);

    if (ea==BADADDR)
        return;

    // get options
    short options = node->altval(-4);

    node->supval(n-1, arrptr[INDEX_COMMENT], MAXSTR);
    
    // get comment type
    int cmttype = GetCommentType(ea);

    
    //qsnprintf(arrptr[INDEX_ADDRESS], MAXSTR, "%08a", ea);
    get_nice_colored_name(ea, arrptr[INDEX_ADDRESS], MAXSTR, GNCN_NOCOLOR | GNCN_NOFUNC | GNCN_NOLABEL);
    
    if (cmttype == INST_NORM_COMMENT)
        qstrncpy(arrptr[INDEX_CMT_TYPE], ".", MAXSTR);
    else
        qstrncpy(arrptr[INDEX_CMT_TYPE], "R", MAXSTR);


    if (cmttype == PROC_COMMENT)
    {
        // don't put code or data, just name of the procedure	     
        if (options & SHOW_FUNC_DECLART & INST_REP_COMMENT)
            print_type(ea, arrptr[INDEX_CODE_DATA], MAXSTR, true);

        if (!strlen(arrptr[INDEX_CODE_DATA]))
            get_func_name(ea, arrptr[INDEX_CODE_DATA], MAXSTR);

    }
    else
    {
        char info[MAXSTR]={0};
        char name[MAXSTR]={0};
        
        //get single line representation of the disasm/data
        //SetComment(ea, "");
        generate_disasm_line(ea, info, MAXSTR, 0);
        tag_remove(info, info, MAXSTR);
        //SetComment(ea, arrptr[INDEX_COMMENT]);

        int quotes=0;
        //remove comment from our instruction text
        for(int i = 0; i< strlen(info); i++)
        {
            //in .net we can have instructions like
            // ldstr "http://www.datarescue.com"
            // so we skip strings
            if (info[i]== '"')
                quotes = (quotes+1)%2;
            else
            {
                if ((info[i] == ';') || (info[i] == '/' && info[i+1] == '/' && quotes==0))
                {
                    info[i] = 0x0;
                    break;
                }
            }
            
        }
                
        // if its data show its name also
        // e.g: "dada db 0" will be shown instead of "db 0"
        flags_t tflag = getFlags(ea);
        
        if (!isCode(tflag) && has_any_name(tflag))
        {
            //get name of variable
            get_name(BADADDR, ea, name, MAXSTR);
            
            //format(arrptr[INDEX_CODE_DATA], name, info);
            
            if(strlen(name) < VAR_LEN)
                addblanks(name, VAR_LEN);

            qsnprintf(arrptr[INDEX_CODE_DATA], MAXSTR, "%s %s", name, info);
            trim(arrptr[INDEX_CODE_DATA]);
            
        }
        else
            // copy instruction
            qstrncpy(arrptr[INDEX_CODE_DATA], info, MAXSTR);
    }    
}

//////////////////////////////////////////////////////////////////////////
//  btEnter
//
//  * Function that is called when the user hits/selects Enter
//////////////////////////////////////////////////////////////////////////
static void idaapi btEnter(void *obj,ulong n)
{
    //msg("enter(%u)\n",n);
    netnode *node = (netnode *)obj;
    jumpto(node->altval(n-1));
}


//////////////////////////////////////////////////////////////////////////
//  btEdit
//
//  * Function that is called when the user hits/selects Edit
//////////////////////////////////////////////////////////////////////////
static void idaapi btEdit(void *obj,ulong n)
{
    //msg("edit(%u)\n",n);
    char oldcmt[MAXSTR];
    char newcmt[MAXSTR];
    
    netnode *node = (netnode *)obj;   

    if (n==0)
        return;
    
    ea_t ea = node->altval(n-1);
    if (ea==BADADDR)
        return;
    
    GetComment(ea, oldcmt, false);

    if (asktext(MAXSTR, newcmt, oldcmt, "Please enter comment:"))
        SetComment(ea, newcmt);

    if (strlen(newcmt) == 0)
        //no comment anymore
        node->altset(-5, MOD_DELETE);
    else
        node->altset(-5, MOD_EDIT);
    refresh_idaview_anyway();
}
//////////////////////////////////////////////////////////////////////////
//	btDelete
//
//   * Function that is called when the user hits/selects Delete 
//////////////////////////////////////////////////////////////////////////
ulong idaapi btDelete(void *obj,ulong n)
{
    //msg("delete(%u)\n",n);

    if (n==0)
        return n;
    
    netnode *node = (netnode *)obj;
    ea_t ea = node->altval(n-1);

    // get comment type
    if (ea==BADADDR)
        return n;

    SetComment(ea, "");
    node->altset(-5, MOD_DELETE);
    refresh_idaview_anyway();
    return n-1;
}

//////////////////////////////////////////////////////////////////////////
//	btDestroy
//
//   * Function that is called when the window is closed
//////////////////////////////////////////////////////////////////////////
static void idaapi btDestroy(void *obj)
{

    netnode *node = (netnode *)obj;
    node->kill();

    //decrement window number count
    winCount--;
}

//////////////////////////////////////////////////////////////////////////
//	getIcon
//
//   * Function that is called when list needs to show an icon for a line
//////////////////////////////////////////////////////////////////////////
int idaapi getIcon(void *obj,ulong n)
{
    netnode *node = (netnode *)obj;

    if (n==0)
        return n;

    ea_t paddr = node->altval(n-1);

    // get comment type
    int cmttype = GetCommentType(paddr);

    if (cmttype == NO_COMMENT)
        return ICON_NO_CMT;

    if (isCode(getFlags(paddr)))
    {
        if (cmttype == PROC_COMMENT)
            return ICON_PROC_CMT;
        else
            return ICON_CODE_CMT;
    }

    if (isData(getFlags(paddr)))
        return ICON_DATA_CMT;

    if (isUnknown(getFlags(paddr)))
        return ICON_UNKN_CMT;
   
}

//////////////////////////////////////////////////////////////////////////
//	btDestroy
//
//   * Function that is called when list needs update a line
//////////////////////////////////////////////////////////////////////////
ulong idaapi getUpdate(void *obj,ulong n)
{
    //msg("update(%u)\n", n);
    netnode *node = (netnode *)obj;

    if (n==0)
        return n;

    ea_t ea = node->altval(n-1);
    
    // get comment type
    if (ea==BADADDR)
        return n;
    
    short last_action = node->altval(-5);

    switch(last_action){
    case MOD_UPDATE:
        //msg("MOD_UPDATE(%u)\n", n);
        GetCommentAddress(node, false);
        break;
    case MOD_EDIT:        
        {
            //msg("MOD_EDIT(%u)\n", n);
            //we just set new comment in our list
            char cmt[MAXSTR];
            GetComment(ea, cmt, true);
            node->supset(n-1, &cmt, sizeof(cmt));
            node->altset(-5, MOD_FAST_UPDATE);
            return n;
        }
        break;
    case MOD_DELETE:
        {
            //msg("MOD_DELETE(%u)\n", n);
            int cmtcount = node->altval(-1);
            node->altset(-1, node->altval(-1)-1);       //decrease cmt count
            node->altset(-5, MOD_UPDATE);    
            if (n != cmtcount)     
            {
                node->altshift(n, n-1, cmtcount- n);
                node->supshift(n, n-1, cmtcount- n);
                return n;
            }
            return n-1;
        }
    case MOD_FAST_UPDATE:
        //msg("MOD_FADT_UPDATE(%u)\n", n);
        // after a fast update there is a normal update
        node->altset(-5, MOD_UPDATE);
        return n;
        break;

    default:
        //msg("MOD_DEFAULT(%u)\n", n);
        GetCommentAddress(node, false);
        break;

    }

   return n;
}

//////////////////////////////////////////////////////////////////////////
//	run
//
//   * main function of the plugin
//////////////////////////////////////////////////////////////////////////

void run(int arg)
{
    char wtitle[MAXSTR];
    segment_t *cmtseg;
    netnode *node = new netnode;
    

    if (!autoIsOk())
    {
        warning("IDA is still analysing your file!\nPlugin will start after autoanalysis is finished.");
        autoWait();
    }
    
    if (!isInited) ShowBanner();
    isInited = true;
    

#ifdef _DEBUG_
    if(arg == -1)
    {
        PLUGIN.flags |= PLUGIN_UNL;
        msg("Unloading CommentViewer plugin...\n");
        return;
    }
#endif

    if (arg == 0)    // do full search
    {
        if (!winCount)
            qstrncpy(wtitle, "Comment Viewer - [FULL]", MAXSTR);
        else
            qsnprintf(wtitle, MAXSTR, "[%i] Comment Viewer - [FULL]", winCount);


        node->create(wtitle);
        node->altset(-2, inf.minEA);          // max address
        node->altset(-3, inf.maxEA);          // min address
    }
    else
    {
        if (arg == 1)   //search only on given segment
        {            
            cmtseg = choose_segm("Please choose segment to get comments from:", get_screen_ea());
            if (cmtseg == NULL) 
                return;

            char segname[MAXSTR];
            get_true_segm_name(cmtseg, segname, MAXSTR);
            
            if (!winCount)
                qsnprintf(wtitle, MAXSTR, "Comment Viewer - [%.8s]", segname);
            else
                qsnprintf(wtitle, MAXSTR, "[%i] Comment Viewer - [%.8s]", winCount, segname);

            node->create(wtitle);
            node->altset(-2, cmtseg->startEA);          //start of segment
            node->altset(-3, cmtseg->endEA);            //end of segment
        }
        else
            return;       
    }

    //get options to use
    int result = AskUsingForm_c(OPTIONS_FORM, &opt1, &opt2);
    
    if (!result)
        return;
    
    short options =  (opt2 << 2) + opt1;
    node->altset(-4, options);
    node->altset(-5, MOD_UPDATE);
    
    //increment number of windows
    winCount++;

    GetCommentAddress(node, true);
    
    // now open the window
    choose2(false,          // non-modal window
        -1, -1, -1, -1,     // position is determined by Windows
        node,               // pass the created netnode to the window
        qnumber(header),    // number of columns
        widths,             // widths of columns
        getSize,            // function that returns number of lines
        getDescription,     // function that generates a line
        wtitle,              // window title
        160,                // use the default icon for the window
        0,                 // position the cursor on the first line
        btDelete,           // "kill" callback
        NULL,               // "new" callback
        getUpdate,          // "update" callback
        btEdit,             // "edit" callback
        btEnter,            // function to call when the user pressed Enter
        btDestroy,          // function to call when the window is closed
        NULL,               // use default popup menu items
        getIcon);           // use the same icon for all line
}

//--------------------------------------------------------------------------
char comment[] = "Show repeatable comments in current database.";

char help[] =
"This plugin shows all the repeatable comments in your database.\n";


//--------------------------------------------------------------------------
// This is the preferred name of the plugin module in the menu system
// The preferred name may be overwritten in plugins.cfg file

char wanted_name[] = "Comment Viewer";


// This is the preferred hotkey for the plugin module
// The preferred hotkey may be overwritten in plugins.cfg file
// Note: IDA won't tell you if the hotkey is not correct
//       It will just disable the hotkey.

char wanted_hotkey[] = "Ctrl-8";


//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
    IDP_INTERFACE_VERSION,
        0,                      // plugin flags
        initPlugin,                   // initialize
        
        termPlugin,                   // terminate. this pointer may be NULL.
        
        run,                    // invoke plugin
        
        comment,                // long comment about the plugin
                                // it could appear in the status line
                                // or as a hint
        
        help,                   // multi-line help about the plugin
        
        wanted_name,            // the preferred short name of the plugin
        wanted_hotkey           // the preferred hotkey to run the plugin
};
