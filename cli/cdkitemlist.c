/* $Id: cdkitemlist.c,v 1.11 2011/05/16 22:53:13 tom Exp $ */

#include <cdk_test.h>

#ifdef XCURSES
char *XCursesProgramName="cdkitemlist";
#endif

/*
 * Declare file local prototypes.
 */
static int widgetCB (EObjectType cdktype, void *object, void *clientData, chtype key);

/*
 * Define file local variables.
 */
static char *FPUsage = "-l List | -f filename [-d Default Item] [-T Title] [-L Label] [-B Buttons] [-O Output File] [-X X Position] [-Y Y Position] [-N] [-S]";

/*
 *
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkScreen		= 0;
   CDKITEMLIST *widget		= 0;
   CDKBUTTONBOX *buttonWidget	= 0;
   WINDOW *cursesWindow		= 0;
   chtype *holder		= 0;
   char *answer			= 0;
   char *CDK_WIDGET_COLOR	= 0;
   char *temp			= 0;
   int ret			= 0;
   int buttonCount		= 0;
   int selection		= 0;
   int shadowHeight		= 0;
   FILE *fp			= stderr;
   char **itemlistList		= 0;
   char **buttonList		= 0;
   int itemlistLines, j1, j2;

   CDK_PARAMS params;
   boolean boxWidget;
   boolean shadowWidget;
   char *buttons;
   char *filename;
   char *label;
   char *list;
   char *outputFile;
   char *title;
   int defaultItem;
   int xpos;
   int ypos;

   CDKparseParams(argc, argv, &params, "d:f:l:B:L:O:T:" CDK_MIN_PARAMS);

   xpos         = CDKparamValue(&params, 'X', CENTER);
   ypos         = CDKparamValue(&params, 'Y', CENTER);
   boxWidget    = CDKparamValue(&params, 'N', TRUE);
   shadowWidget = CDKparamValue(&params, 'S', FALSE);

   defaultItem  = CDKparamValue(&params, 'd', 0);
   filename     = CDKparamString(&params, 'f');
   list         = CDKparamString(&params, 'l');
   buttons      = CDKparamString(&params, 'B');
   label        = CDKparamString(&params, 'L');
   outputFile   = CDKparamString(&params, 'O');
   title        = CDKparamString(&params, 'T');

   /* If the user asked for an output file, try to open it. */
   if (outputFile != 0)
   {
      if ((fp = fopen (outputFile, "w")) == 0)
      {
	 fprintf (stderr, "%s: Can not open output file %s\n", argv[0], outputFile);
	 ExitProgram (CLI_ERROR);
      }
   }

   /* Did they provide a list of items. */
   if (list == 0)
   {
      /* Maybe they gave a filename to use to read. */
      if (filename != 0)
      {
	 /* Read the file in. */
	 itemlistLines = CDKreadFile (filename, &itemlistList);

	 /* Check if there was an error. */
	 if (itemlistLines == -1)
	 {
	    fprintf (stderr, "Error: Could not open the file '%s'.\n", filename);
	    ExitProgram (CLI_ERROR);
	 }
      }
      else
      {
	 /* They didn't provide anything. */
	 fprintf (stderr, "Usage: %s %s\n", argv[0], FPUsage);
	 ExitProgram (CLI_ERROR);
      }
   }
   else
   {
      /* Split the itemlist lines up. */
      itemlistList = CDKsplitString (list, '\n');
      itemlistLines = (int) CDKcountStrings (itemlistList);
   }

   /* Start curses. */
   cursesWindow = initscr();

   /* Create the CDK screen. */
   cdkScreen = initCDKScreen (cursesWindow);

   /* Start color. */
   initCDKColor();

   /* Check if the user wants to set the background of the main screen. */
   if ((temp = getenv ("CDK_SCREEN_COLOR")) != 0)
   {
      holder = char2Chtype (temp, &j1, &j2);
      wbkgd (cdkScreen->window, holder[0]);
      wrefresh (cdkScreen->window);
      freeChtype (holder);
   }

   /* Get the widget color background color. */
   if ((CDK_WIDGET_COLOR = getenv ("CDK_WIDGET_COLOR")) == 0)
   {
      CDK_WIDGET_COLOR = 0;
   }

   /* Create the item list. */
   widget = newCDKItemlist (cdkScreen, xpos, ypos,
				title, label,
				itemlistList, itemlistLines,
				defaultItem,
				boxWidget, shadowWidget);

   /* Make sure we could create the widget. */
   if (widget == 0)
   {
      CDKfreeStrings(itemlistList);

      destroyCDKScreen (cdkScreen);
      endCDK();

      fprintf (stderr, "Error: Could not create the item list. Is the window too small?\n");

      ExitProgram (CLI_ERROR);
   }

   /* Split the buttons if they supplied some. */
   if (buttons != 0)
   {
      buttonList = CDKsplitString (buttons, '\n');
      buttonCount = (int) CDKcountStrings (buttonList);

      /* We need to create a buttonbox widget. */
      buttonWidget = newCDKButtonbox (cdkScreen,
					getbegx (widget->win),
					getbegy (widget->win) + widget->boxHeight - 1,
					1, widget->boxWidth - 1,
					0, 1, buttonCount,
					buttonList, buttonCount,
					A_REVERSE, boxWidget, FALSE);
      CDKfreeStrings (buttonList);

      setCDKButtonboxULChar (buttonWidget, ACS_LTEE);
      setCDKButtonboxURChar (buttonWidget, ACS_RTEE);

      /*
       * We need to set the lower left and right
       * characters of the widget.
       */
      setCDKItemlistLLChar (widget, ACS_LTEE);
      setCDKItemlistLRChar (widget, ACS_RTEE);

      /*
       * Bind the Tab key in the widget to send a
       * Tab key to the button box widget.
       */
      bindCDKObject (vITEMLIST, widget, KEY_TAB, widgetCB, buttonWidget);
      bindCDKObject (vITEMLIST, widget, CDK_NEXT, widgetCB, buttonWidget);
      bindCDKObject (vITEMLIST, widget, CDK_PREV, widgetCB, buttonWidget);

      /* Check if the user wants to set the background of the widget. */
      setCDKButtonboxBackgroundColor (buttonWidget, CDK_WIDGET_COLOR);

      /* Draw the button widget. */
      drawCDKButtonbox (buttonWidget, boxWidget);
   }

   /*
    * If the user asked for a shadow, we need to create one.  Do this instead
    * of using the shadow parameter because the button widget is not part of
    * the main widget and if the user asks for both buttons and a shadow, we
    * need to create a shadow big enough for both widgets.  Create the shadow
    * window using the widgets shadowWin element, so screen refreshes will draw
    * them as well.
    */
   if (shadowWidget == TRUE)
   {
      /* Determine the height of the shadow window. */
      shadowHeight = (buttonWidget == 0 ?
			widget->boxHeight :
			widget->boxHeight + buttonWidget->boxHeight - 1);

      /* Create the shadow window. */
      widget->shadowWin = newwin (shadowHeight,
					widget->boxWidth,
					getbegy (widget->win) + 1,
					getbegx (widget->win) + 1);

      /* Make sure we could have created the shadow window. */
      if (widget->shadowWin != 0)
      {
	 widget->shadow = TRUE;

	 /*
	  * We force the widget and buttonWidget to be drawn so the
	  * buttonbox widget will be drawn when the widget is activated.
	  * Otherwise the shadow window will draw over the button widget.
	  */
	 drawCDKItemlist (widget, ObjOf(widget)->box);
	 eraseCDKButtonbox (buttonWidget);
	 drawCDKButtonbox (buttonWidget, ObjOf(buttonWidget)->box);
      }
   }

   /* Check if the user wants to set the background of the widget. */
   setCDKItemlistBackgroundColor (widget, CDK_WIDGET_COLOR);

   /* Activate the item list. */
   ret = activateCDKItemlist (widget, 0);

   /* If there were buttons, get the button selected. */
   if (buttonWidget != 0)
   {
      selection = buttonWidget->currentButton;
      destroyCDKButtonbox (buttonWidget);
   }

   destroyCDKItemlist (widget);
   destroyCDKScreen (cdkScreen);
   endCDK();

   /* Print out the answer. */
   if (ret >= 0)
   {
      holder = char2Chtype (itemlistList[ret], &j1, &j2);
      answer = chtype2Char (holder);
      fprintf (fp, "%s\n", answer);
      freeChar (answer);
      freeChtype (holder);
   }
   CDKfreeStrings(itemlistList);
   fclose (fp);

   /* Exit with the answer. */
   ExitProgram (selection);
}

static int widgetCB (EObjectType cdktype GCC_UNUSED, void *object GCC_UNUSED, void *clientData, chtype key)
{
   CDKBUTTONBOX *buttonbox = (CDKBUTTONBOX *)clientData;
   injectCDKButtonbox (buttonbox, key);
   return (TRUE);
}
