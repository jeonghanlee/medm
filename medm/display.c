/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"
#include <Xm/MwmUtil.h>

static void displayGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable displayDlDispatchTable = {
         createDlDisplay,
         NULL,
         executeDlDisplay,
         writeDlDisplay,
         NULL,
         displayGetValues,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL};
  

/*
 * create and fill in widgets for display
 */

DisplayInfo *createDisplay()
{
  DisplayInfo *displayInfo;
  DlElement *dlElement;
  Arg args[10];

/* clear currentDisplayInfo - not really one yet */
  currentDisplayInfo = NULL;
  initializeGlobalResourceBundle();

  if (!(displayInfo = allocateDisplayInfo())) return NULL;

/* general scheme: update  globalResourceBundle, then do creates */
  globalResourceBundle.x = 0;
  globalResourceBundle.y = 0;
  globalResourceBundle.width = DEFAULT_DISPLAY_WIDTH;
  globalResourceBundle.height = DEFAULT_DISPLAY_HEIGHT;
  strcpy(globalResourceBundle.name,DEFAULT_FILE_NAME);
  displayInfo->dlFile = createDlFile(displayInfo);
  dlElement = createDlDisplay(NULL);
  if (dlElement) {
    DlDisplay *dlDisplay = dlElement->structure.display;
    dlDisplay->object.x = globalResourceBundle.x;
    dlDisplay->object.y = globalResourceBundle.y;
    dlDisplay->object.width = globalResourceBundle.width;
    dlDisplay->object.height = globalResourceBundle.height;
    dlDisplay->clr = globalResourceBundle.clr;
    dlDisplay->bclr = globalResourceBundle.bclr;
    appendDlElement(displayInfo->dlElementList,dlElement);
  } else {
    /* cleanup up displayInfo */
    return NULL;
  }
  displayInfo->dlColormap = createDlColormap(displayInfo);
  dmTraverseDisplayList(displayInfo);
  XtPopup(displayInfo->shell,XtGrabNone);

  currentDisplayInfo = displayInfo;

  return(displayInfo);
}

#define DISPLAY_DEFAULT_X 10
#define DISPLAY_DEFAULT_Y 10
 
DlElement *createDlDisplay(DlElement *p)
{
  DlDisplay *dlDisplay;
  DlElement *dlElement;
 
 
  dlDisplay = (DlDisplay *) malloc(sizeof(DlDisplay));
  if (!dlDisplay) return 0;
  if (p) {
    *dlDisplay = *p->structure.display; 
  } else {
    objectAttributeInit(&(dlDisplay->object));
    dlDisplay->object.x = DISPLAY_DEFAULT_X;
    dlDisplay->object.y = DISPLAY_DEFAULT_Y;
    dlDisplay->clr = 0;
    dlDisplay->bclr = 1;
    dlDisplay->cmap[0] = '\0';
  }
 
  if (!(dlElement = createDlElement(DL_Display,
                                    (XtPointer)dlDisplay,
                                    &displayDlDispatchTable))) {
    free(dlDisplay);
  }
 
  return(dlElement);
}

/*
 * this execute... function is a bit unique in that it can properly handle
 *  being called on extant widgets (drawingArea's/shells) - all other
 *  execute... functions want to always create new widgets (since there
 *  is no direct record of the widget attached to the object/element)
 *  {this can be fixed to save on widget create/destroy cycles later}
 */
void executeDlDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
  DlColormap *dlColormap;
  Arg args[12];
  int n;
  DlDisplay *dlDisplay = dlElement->structure.display;

 
/* set the display's foreground and background colors */
  displayInfo->drawingAreaBackgroundColor = dlDisplay->bclr;
  displayInfo->drawingAreaForegroundColor = dlDisplay->clr;
 
/* from the DlDisplay structure, we've got drawingArea's dimensions */
  n = 0;
  XtSetArg(args[n],XmNwidth,(Dimension)dlDisplay->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlDisplay->object.height); n++;
  XtSetArg(args[n],XmNborderWidth,(Dimension)0); n++;
  XtSetArg(args[n],XmNmarginWidth,(Dimension)0); n++;
  XtSetArg(args[n],XmNmarginHeight,(Dimension)0); n++;
  XtSetArg(args[n],XmNshadowThickness,(Dimension)0); n++;
  XtSetArg(args[n],XmNresizePolicy,XmRESIZE_NONE); n++;
/* N.B.: don't use userData resource since it is used later on for aspect
 *   ratio-preserving resizes */
 
  if (displayInfo->drawingArea == NULL) {
 
      displayInfo->drawingArea = XmCreateDrawingArea(displayInfo->shell,
    "displayDA",args,n);
    /* add expose & resize  & input callbacks for drawingArea */
     XtAddCallback(displayInfo->drawingArea,XmNexposeCallback,
    (XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
     XtAddCallback(displayInfo->drawingArea,XmNresizeCallback,
    (XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
     XtManageChild(displayInfo->drawingArea);
 
   /*
    * and if in EDIT mode...
    */
    if (displayInfo->traversalMode == DL_EDIT) {
      /* handle input (arrow keys) */
      XtAddCallback(displayInfo->drawingArea,XmNinputCallback,
        (XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
      /* and handle button presses and enter windows */
      XtAddEventHandler(displayInfo->drawingArea,ButtonPressMask,False,
        handleButtonPress,(XtPointer)displayInfo);
      XtAddEventHandler(displayInfo->drawingArea,EnterWindowMask,False,
        (XtEventHandler)handleEnterWindow,(XtPointer)displayInfo);
 
    } else
    if (displayInfo->traversalMode == DL_EXECUTE) {
 
/*
 *  MDA --- HACK to fix DND visuals problem with SUN server
 *   Note: This call is in here strictly to satisfy some defect in
 *    the MIT and other X servers for SUNOS machines
 *     This is completely unnecessary for HP, DEC, NCD, ...
 */
      XtSetArg(args[0],XmNdropSiteType,XmDROP_SITE_COMPOSITE);
      XmDropSiteRegister(displayInfo->drawingArea,args,1);
      /* and handle button presses and enter windows */
      XtAddEventHandler(displayInfo->drawingArea,ButtonPressMask,False,
        popupMenu,(XtPointer)displayInfo);
 
 
      /* add in drag/drop translations */
      XtOverrideTranslations(displayInfo->drawingArea,parsedTranslations);
    }
 
  } else  {
      XtSetValues(displayInfo->drawingArea,args,n);
  }
 
 
 
/* wait to realize the shell... */
  n = 0;
  if (!XtIsRealized(displayInfo->shell)) {  /* only position first time */
    XtSetArg(args[n],XmNx,(Position)dlDisplay->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlDisplay->object.y); n++;
  }
  XtSetArg(args[n],XmNallowShellResize,(Boolean)TRUE); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlDisplay->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlDisplay->object.height); n++;
  XtSetArg(args[n],XmNiconName,displayInfo->dlFile->name); n++;
  XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  XtSetValues(displayInfo->shell,args,n);
  medmSetDisplayTitle(displayInfo);
  XtRealizeWidget(displayInfo->shell);
 
 
/* if there is an external colormap file specification, parse/execute it now
*/
 
  if (strlen(dlDisplay->cmap) > (size_t)1)  {
 
    if (dlColormap = parseAndExtractExternalColormap(displayInfo,
        dlDisplay->cmap)) {
      executeDlColormap(displayInfo,dlColormap);
    } else {
      fprintf(stderr,
        "\nexecuteDlDisplay: can't parse and execute external colormap %s",
        dlDisplay->cmap);
      medmCATerminate();
      dmTerminateX();
      exit(-1);
    }
  } else {
    executeDlColormap(displayInfo,displayInfo->dlColormap);
  }
}

void writeDlDisplay(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
  int i;
  char indent[16];
  DlDisplay *dlDisplay = dlElement->structure.display;
 
  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';
 
  fprintf(stream,"\n%sdisplay {",indent);
  writeDlObject(stream,&(dlDisplay->object),level+1);
  fprintf(stream,"\n%s\tclr=%d",indent,dlDisplay->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlDisplay->bclr);
  fprintf(stream,"\n%s\tcmap=\"%s\"",indent,dlDisplay->cmap);
  fprintf(stream,"\n%s}",indent);
 
}

static void displayGetValues(ResourceBundle *pRCB, DlElement *p) {
  DlDisplay *dlDisplay = p->structure.display;
  medmGetValues(pRCB,
    X_RC,          &(dlDisplay->object.x),
    Y_RC,          &(dlDisplay->object.y),
    WIDTH_RC,      &(dlDisplay->object.width),
    HEIGHT_RC,     &(dlDisplay->object.height),
    CLR_RC,        &(dlDisplay->clr),
    BCLR_RC,       &(dlDisplay->bclr),
    CMAP_RC,       &(dlDisplay->cmap),
    -1);
    currentDisplayInfo->drawingAreaBackgroundColor =
                                globalResourceBundle.bclr;
    currentDisplayInfo->drawingAreaForegroundColor =
                                globalResourceBundle.clr;
    /* and resize the shell */
    XtVaSetValues(currentDisplayInfo->shell,
                XmNx,dlDisplay->object.x,
                XmNy,dlDisplay->object.y,
                XmNwidth,dlDisplay->object.width,
                XmNheight,dlDisplay->object.height,NULL);

}
