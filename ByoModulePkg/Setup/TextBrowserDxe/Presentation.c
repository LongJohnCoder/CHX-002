/** @file
Utility functions for UI presentation.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"

BOOLEAN            mHiiPackageListUpdated;
BOOLEAN            mReloadFormset = FALSE;
UI_MENU_SELECTION  *gCurrentSelection;
EFI_HII_HANDLE     mCurrentHiiHandle = NULL;
EFI_GUID           mCurrentFormSetGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
UINT16             mCurrentFormId = 0;
UINT16             gMenuItemCounts;
UINT16             mParentFormid = 0;
BOOLEAN          bDrawHelpMessage = TRUE;

#define MAX_TITLE_ITEMS    7
#define NO_HELP_MESSAGE_FORM    0xBB00

/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
ClearLines (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
  )
{
  CHAR16  *Buffer;
  UINTN   Row;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  //
  // Set foreground and background as defined
  //
  gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);

  //
  // Much faster to buffer the long string instead of print it a character at a time
  //
  SetUnicodeMem (Buffer, RightColumn - LeftColumn, L' ');

  //
  // Clear the desired area with the appropriate foreground/background
  //
  for (Row = TopRow; Row <= BottomRow; Row++) {
    PrintStringAt (LeftColumn, Row, Buffer);
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, LeftColumn, TopRow);

  FreePool (Buffer);
  return ;
}

/**
  Repair black line on the top and bottom..

**/
VOID
RepairScreen ()
{
  EFI_STATUS    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput = NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    BackGroundBlt_Blue =   {0x98, 0x00, 0x00, 0x00};  // LIGHTBLUE

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (!EFI_ERROR(Status)) {
    //
    // paint first line to repaire Black line on top.
    //
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &BackGroundBlt_Blue,
                        EfiBltVideoFill,
                        0, //SourceX
                        0, //SourceY
                        0,//DestinationX
                        0,//DestinationY
                        EFI_GLYPH_WIDTH * gScreenDimensions.RightColumn, //Width
                        EFI_GLYPH_HEIGHT, //Height
                        0
                        );

    //
    // paint last line to repaire Black line on bottom.
    //
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        &BackGroundBlt_Blue,
                        EfiBltVideoFill,
                        0, //SourceX
                        0, //SourceY
                        0,//DestinationX
                        (GraphicsOutput->Mode->Info->VerticalResolution - EFI_GLYPH_HEIGHT), //DestinationY
                        EFI_GLYPH_WIDTH * gScreenDimensions.RightColumn, //Width
                        EFI_GLYPH_HEIGHT, //Height
                        0
                        );

  }

  return ;
}

/**
  Clear retangle with Blt of GOP, It's replace of ClearLines to improve performance.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
GopBltArea (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
  )
{
  EFI_STATUS    Status;
  UINT8    UCREnable;

  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput = NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGroundBlt = NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    BackGroundBlt_Lightgray = {0xb0, 0xb8, 0xb0, 0x00};  // LIGHTGRAY
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    BackGroundBlt_Blue =   {0x98, 0x00, 0x00, 0x00};  // LIGHTBLUE
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    BackGroundBlt_Black = {0x00, 0x00, 0x00, 0x00}; // BLACK

  UCREnable = PcdGet8 (PcdUCREnable);
  if (1 == UCREnable) {
    ClearLines(LeftColumn, RightColumn, TopRow, BottomRow, TextAttribute);
    return;
  }

  if (EFI_BACKGROUND_LIGHTGRAY == (TextAttribute & EFI_BACKGROUND_LIGHTGRAY) ) {
    BackGroundBlt = &BackGroundBlt_Lightgray;
  } else if (EFI_BACKGROUND_BLUE == (TextAttribute & EFI_BACKGROUND_BLUE) ) {
    BackGroundBlt = &BackGroundBlt_Blue;
  } else {
    BackGroundBlt = &BackGroundBlt_Black;
  }

  //
  // GOP or UGA.
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (!EFI_ERROR(Status)) {
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        BackGroundBlt,
                        EfiBltVideoFill,
                        0,
                        0,
                        (GraphicsOutput->Mode->Info->HorizontalResolution - gScreenDimensions.RightColumn * EFI_GLYPH_WIDTH)/2 + EFI_GLYPH_WIDTH * LeftColumn,
                        (GraphicsOutput->Mode->Info->VerticalResolution - gScreenDimensions.BottomRow * EFI_GLYPH_HEIGHT)/2 + EFI_GLYPH_HEIGHT * TopRow,
                        EFI_GLYPH_WIDTH * (RightColumn - LeftColumn),
                        EFI_GLYPH_HEIGHT* (BottomRow - TopRow + 1),
                        0
                        );
  }

  if (NULL == GraphicsOutput) {
    ClearLines(LeftColumn, RightColumn, TopRow, BottomRow, TextAttribute);
  }

  return ;
}


/**
  Concatenate a narrow string to another string.

  @param Destination The destination string.
  @param Source      The source string. The string to be concatenated.
                     to the end of Destination.

**/
VOID
NewStrCat (
  IN OUT CHAR16               *Destination,
  IN     CHAR16               *Source
  )
{
  UINTN Length;

  for (Length = 0; Destination[Length] != 0; Length++)
    ;

  //
  // We now have the length of the original string
  // We can safely assume for now that we are concatenating a narrow value to this string.
  // For instance, the string is "XYZ" and cat'ing ">"
  // If this assumption changes, we need to make this routine a bit more complex
  //
  Destination[Length] = NARROW_CHAR;
  Length++;

  StrCpy (Destination + Length, Source);
}

/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetStringWidth (
  IN CHAR16               *String
  )
{
  UINTN Index;
  UINTN Count;
  UINTN IncrementValue;

  ASSERT (String != NULL);
  if (String == NULL) {
    return 0;
  }

  Index           = 0;
  Count           = 0;
  IncrementValue  = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for (;
         (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
         Index++, Count = Count + IncrementValue
        )
      ;

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }
    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  //
  // Increment by one to include the null-terminator in the size
  //
  Count++;

  return Count * sizeof (CHAR16);
}

/**
  This function displays the page frame.

  @param  Selection              Selection contains the information about 
                                 the Selection, form and formset to be displayed.
                                 Selection action may be updated in retrieve callback.
**/
VOID
DisplayPageFrame (
  IN UI_MENU_SELECTION    *Selection
  )
{
  UINTN                  Index;
  UINT8                  Line;
  UINT8                  Alignment;
  CHAR16                 *Buffer;
  CHAR16                 *StrFrontPageBanner;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  UINT8                  RowIdx;
  UINT8                  ColumnIdx;
  CHAR16               *SubMenuTitle;  
  UINTN                  SubMenuTitleLength;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &LocalScreen.RightColumn, &LocalScreen.BottomRow);
  
  if (Selection->Form->ModalForm) {
    return;
  }

  if ((gClassOfVfr & FORMSET_CLASS_PLATFORM_SETUP) == FORMSET_CLASS_PLATFORM_SETUP) {
    //
    // For Platform Setup.
    //
    if (bDrawSetupBackground) {
      DrawSetupBackground();
    }
    //
    // Help Message Background.
    //
    if ((Selection->Form->FormId & NO_HELP_MESSAGE_FORM) == NO_HELP_MESSAGE_FORM) {     
      //
      // Not show help message when setup formid is 0xBBXX.
      //
      bDrawHelpMessage = FALSE;
    } else {
      bDrawHelpMessage = TRUE;
      DrawHelpMessageBackground ();
    }

    //
    // line 4 which may be redrawed.
    //
    if (bDrawHelpMessage) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);
    PrintCharAt(LocalScreen.LeftColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL);
    PrintCharAt(gMiddleVerticalLineColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_RIGHT);
    }
    if((Selection->FormId == gFirstFormId) && (IsByoMainFormset(Selection->Handle))) {
      DrawFormTitleBar (Selection, TRUE);
    }else{
      DrawFormTitleBar (Selection, FALSE);
      if (bDrawHelpMessage) {
      //
      // Draw sub form title, line 3.
      //
      gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE|FIELD_BACKGROUND);
      GopBltArea (
            LocalScreen.LeftColumn+1,
            LocalScreen.RightColumn - gHelpBlockWidth,
            LocalScreen.TopRow + 3,
            LocalScreen.TopRow + 3,
            EFI_BLUE|FIELD_BACKGROUND
            );
      SubMenuTitle = NULL;    
      SubMenuTitle = GetToken (Selection->Form->FormTitle, Selection->Handle);
      if (NULL == SubMenuTitle) {
        SubMenuTitle = L"Sub Item";
      }
      SubMenuTitleLength = GetStringWidth (SubMenuTitle) / 2 - 1;
      if (SubMenuTitleLength > (UINTN)gMiddleVerticalLineColumn -2) {
        SubMenuTitleLength = gMiddleVerticalLineColumn -2;
        while (SubMenuTitle[SubMenuTitleLength] != L' ') {
          SubMenuTitleLength--;
        }
        SubMenuTitle[SubMenuTitleLength] = CHAR_NULL;
      }	  	
      PrintStringAt (
              (gMiddleVerticalLineColumn - SubMenuTitleLength) / 2 + LocalScreen.LeftColumn,
              LocalScreen.TopRow + 3,
              SubMenuTitle
              );   
      FreePool(SubMenuTitle);
      //
      // Bar, line 4.
      //
      PrintCharAt(LocalScreen.LeftColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_RIGHT);
      Buffer = AllocateZeroPool (0x100);
      ASSERT (Buffer != NULL);
      for (Index = 0; Index + 1 < gMiddleVerticalLineColumn; Index++) {
        Buffer[Index] = BOXDRAW_HORIZONTAL;
      }
      PrintString (Buffer);
      FreePool (Buffer);  
      PrintCharAt(gMiddleVerticalLineColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_HORIZONTAL);
      } else {
        //
        // Draw sub form title, line 3.
        //
        gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE|FIELD_BACKGROUND);
        GopBltArea (
              LocalScreen.LeftColumn+1,
              LocalScreen.RightColumn - 1,
              LocalScreen.TopRow + 3,
              LocalScreen.TopRow + 3,
              EFI_BLUE|FIELD_BACKGROUND
              );
        SubMenuTitle = NULL;
        SubMenuTitle = GetToken (Selection->Form->FormTitle, Selection->Handle);
        if (NULL == SubMenuTitle) {
          SubMenuTitle = L"Sub Item";
        }
        SubMenuTitleLength = GetStringWidth (SubMenuTitle) / 2 - 1;
        if (SubMenuTitleLength > (UINTN)gMiddleVerticalLineColumn -2) {
          SubMenuTitleLength = gMiddleVerticalLineColumn -2;
          while (SubMenuTitle[SubMenuTitleLength] != L' ') {
            SubMenuTitleLength--;
          }
          SubMenuTitle[SubMenuTitleLength] = CHAR_NULL;
        }
        PrintStringAt (
                (gMiddleVerticalLineColumn - SubMenuTitleLength) / 2 + LocalScreen.LeftColumn,
                LocalScreen.TopRow + 3,
                SubMenuTitle
                );
        FreePool(SubMenuTitle);
      }
    } 
  } else if ((gClassOfVfr & FORMSET_CLASS_BOOT_MANAGER) == FORMSET_CLASS_BOOT_MANAGER) {
    //
    // For Boot Manager Menu.
    //     Todo: This type maybe needed in future.
    //
    ClearLines (0, LocalScreen.RightColumn, 0, LocalScreen.BottomRow, EFI_BACKGROUND_RED);
  } else {
    //
    // This must be FORMSET_CLASS_FRONT_PAGE.
    //     Todo: This type maybe needed in future.
    //
    ClearLines (0, LocalScreen.RightColumn, 0, LocalScreen.BottomRow, EFI_BACKGROUND_GREEN);
    
    ClearLines (
      LocalScreen.LeftColumn,
      LocalScreen.RightColumn,
      LocalScreen.TopRow,
      FRONT_PAGE_HEADER_HEIGHT - 1 + LocalScreen.TopRow,
      BANNER_TEXT | BANNER_BACKGROUND
      );
    //
    //    for (Line = 0; Line < BANNER_HEIGHT; Line++) {
    //
    for (Line = (UINT8) LocalScreen.TopRow; Line < BANNER_HEIGHT + (UINT8) LocalScreen.TopRow; Line++) {
      //
      //      for (Alignment = 0; Alignment < BANNER_COLUMNS; Alignment++) {
      //
      for (Alignment = (UINT8) LocalScreen.LeftColumn;
           Alignment < BANNER_COLUMNS + (UINT8) LocalScreen.LeftColumn;
           Alignment++
          ) {
        RowIdx = (UINT8) (Line - (UINT8) LocalScreen.TopRow);
        ColumnIdx = (UINT8) (Alignment - (UINT8) LocalScreen.LeftColumn);

        ASSERT (RowIdx < BANNER_HEIGHT);
        ASSERT (ColumnIdx < BANNER_COLUMNS);

        if (gBannerData->Banner[RowIdx][ColumnIdx] != 0x0000) {
          StrFrontPageBanner = GetToken (
                                gBannerData->Banner[RowIdx][ColumnIdx],
                                gFrontPageHandle
                                );
        } else {
          continue;
        }

        switch (Alignment - LocalScreen.LeftColumn) {
        case 0:
          //
          // Handle left column
          //
          PrintStringAt (LocalScreen.LeftColumn + BANNER_LEFT_COLUMN_INDENT, Line, StrFrontPageBanner);
          break;

        case 1:
          //
          // Handle center column
          //
          PrintStringAt (
            LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3,
            Line,
            StrFrontPageBanner
            );
          break;

        case 2:
          //
          // Handle right column
          //
          PrintStringAt (
            LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) * 2 / 3,
            Line,
            StrFrontPageBanner
            );
          break;
        }

        if (NULL != StrFrontPageBanner) {
          FreePool (StrFrontPageBanner);
        }
      }
    }
  }  
  bDrawSetupBackground = FALSE;
}


/**
  Evaluate all expressions in a Form.

  @param  FormSet        FormSet this Form belongs to.
  @param  Form           The Form.

  @retval EFI_SUCCESS    The expression evaluated successfuly

**/
EFI_STATUS
EvaluateFormExpressions (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY       *Link;
  FORM_EXPRESSION  *Expression;

  Link = GetFirstNode (&Form->ExpressionListHead);
  while (!IsNull (&Form->ExpressionListHead, Link)) {
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    Link = GetNextNode (&Form->ExpressionListHead, Link);

    if (Expression->Type == EFI_HII_EXPRESSION_INCONSISTENT_IF ||
        Expression->Type == EFI_HII_EXPRESSION_NO_SUBMIT_IF ||
        Expression->Type == EFI_HII_EXPRESSION_WRITE ||
        (Expression->Type == EFI_HII_EXPRESSION_READ && Form->FormType != STANDARD_MAP_FORM_TYPE)) {
      //
      // Postpone Form validation to Question editing or Form submitting or Question Write or Question Read for nonstandard form.
      //
      continue;
    }

    Status = EvaluateExpression (FormSet, Form, Expression);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/*
+------------------------------------------------------------------------------+
?                                 Setup Page                                  ?
+------------------------------------------------------------------------------+

















+------------------------------------------------------------------------------+
?F1=Scroll Help                 F9=Reset to Defaults        F10=Save and Exit ?
| ^"=Move Highlight          <Spacebar> Toggles Checkbox   Esc=Discard Changes |
+------------------------------------------------------------------------------+
*/

/**


  Display form and wait for user to select one menu option, then return it.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.
  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
DisplayForm (
  IN OUT UI_MENU_SELECTION           *Selection
  )
{
  CHAR16                 *StringPtr;
  UINT16                 MenuItemCount;
  EFI_HII_HANDLE         Handle;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  UINT16                 Width;
  UINTN                  ArrayEntry;
  CHAR16                 *OutputString;
  LIST_ENTRY             *Link;
  FORM_BROWSER_STATEMENT *Statement;
  UINT16                 NumberOfLines;
  EFI_STATUS             Status;
  UI_MENU_OPTION         *MenuOption;
  UINT16                 GlyphWidth;
  
  Handle        = Selection->Handle;
  MenuItemCount = 0;
  ArrayEntry    = 0;
  OutputString  = NULL;

  UiInitMenu ();

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // Evaluate all the Expressions in this Form
  //
  Status = EvaluateFormExpressions (Selection->FormSet, Selection->Form);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Selection->FormEditable = FALSE;
  Link = GetFirstNode (&Selection->Form->StatementListHead);
  while (!IsNull (&Selection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    if (EvaluateExpressionList(Statement->Expression, FALSE, NULL, NULL) <= ExpressGrayOut) {
      StringPtr = GetToken (Statement->Prompt, Handle);
      ASSERT (StringPtr != NULL);

      Width     = GetWidth (Statement, Handle);

      NumberOfLines = 1;
      ArrayEntry = 0;
      GlyphWidth = 1;
      for (; GetLineByWidth (StringPtr, Width, &GlyphWidth,&ArrayEntry, &OutputString) != 0x0000;) {
        //
        // If there is more string to process print on the next row and increment the Skip value
        //
        if (StrLen (&StringPtr[ArrayEntry]) != 0) {
          NumberOfLines++;
        }

        FreePool (OutputString);
      }

      //
      // We are NOT!! removing this StringPtr buffer via FreePool since it is being used in the menuoptions, we will do
      // it in UiFreeMenu.
      //
      MenuOption = UiAddMenuOption (StringPtr, Selection->Handle, Selection->Form, Statement, NumberOfLines, MenuItemCount);
      MenuItemCount++;

      if (MenuOption->IsQuestion && !MenuOption->ReadOnly) {
        //
        // At least one item is not readonly, this Form is considered as editable
        //
        Selection->FormEditable = TRUE;
      }
    }

    Link = GetNextNode (&Selection->Form->StatementListHead, Link);
  }
  gMenuItemCounts = MenuItemCount;

  Status = UiDisplayMenu (Selection);

  UiFreeMenu ();

  return Status;
}

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeBrowserStrings (
  VOID
  )
{
  gEnterString          = GetToken (STRING_TOKEN (ENTER_STRING), gHiiHandle);
  gEnterCommitString    = GetToken (STRING_TOKEN (ENTER_COMMIT_STRING), gHiiHandle);
  gEnterEscapeString    = GetToken (STRING_TOKEN (ENTER_ESCAPE_STRING), gHiiHandle);
  gEscapeString         = GetToken (STRING_TOKEN (ESCAPE_STRING), gHiiHandle);
  gMoveHighlight        = GetToken (STRING_TOKEN (MOVE_HIGHLIGHT), gHiiHandle);
  gMakeSelection        = GetToken (STRING_TOKEN (MAKE_SELECTION), gHiiHandle);
  gDecNumericInput      = GetToken (STRING_TOKEN (DEC_NUMERIC_INPUT), gHiiHandle);
  gHexNumericInput      = GetToken (STRING_TOKEN (HEX_NUMERIC_INPUT), gHiiHandle);
  gToggleCheckBox       = GetToken (STRING_TOKEN (TOGGLE_CHECK_BOX), gHiiHandle);
  gPromptForData        = GetToken (STRING_TOKEN (PROMPT_FOR_DATA), gHiiHandle);
  gPromptForPassword    = GetToken (STRING_TOKEN (PROMPT_FOR_PASSWORD), gHiiHandle);
  gPromptForNewPassword = GetToken (STRING_TOKEN (PROMPT_FOR_NEW_PASSWORD), gHiiHandle);
  gConfirmPassword      = GetToken (STRING_TOKEN (CONFIRM_PASSWORD), gHiiHandle);
  gConfirmError         = GetToken (STRING_TOKEN (CONFIRM_ERROR), gHiiHandle);
  gPassowordInvalid     = GetToken (STRING_TOKEN (PASSWORD_INVALID), gHiiHandle);
  gPressEnter           = GetToken (STRING_TOKEN (PRESS_ENTER), gHiiHandle);
  gEmptyString          = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
  gAreYouSure           = GetToken (STRING_TOKEN (ARE_YOU_SURE), gHiiHandle);
  gYesResponse          = GetToken (STRING_TOKEN (ARE_YOU_SURE_YES), gHiiHandle);
  gNoResponse           = GetToken (STRING_TOKEN (ARE_YOU_SURE_NO), gHiiHandle);
  gMiniString           = GetToken (STRING_TOKEN (MINI_STRING), gHiiHandle);
  gPlusString           = GetToken (STRING_TOKEN (PLUS_STRING), gHiiHandle);
  gMinusString          = GetToken (STRING_TOKEN (MINUS_STRING), gHiiHandle);
  gAdjustNumber         = GetToken (STRING_TOKEN (ADJUST_NUMBER), gHiiHandle);
  gSaveChanges          = GetToken (STRING_TOKEN (SAVE_CHANGES), gHiiHandle);
  gOptionMismatch       = GetToken (STRING_TOKEN (OPTION_MISMATCH), gHiiHandle);
  gFormSuppress         = GetToken (STRING_TOKEN (FORM_SUPPRESSED), gHiiHandle);

  gAdjustNumber         = GetToken (STRING_TOKEN (ADJUST_NUMBER), gHiiHandle);
  gByoSoftWare          = GetToken (STRING_TOKEN (BYO_SOFT_FLAG), gHiiHandle); //R01 -a
  gSelectItem           = GetToken (STRING_TOKEN (STRING_ARROWUP_DOWN), gHiiHandle);
  gMinusPlus            = GetToken (STRING_TOKEN (PLUS_MINUS_STRING), gHiiHandle);
  gChangeValue          = GetToken (STRING_TOKEN (STRING_CHANGE_VALUES), gHiiHandle);
  gFunctionNine         = GetToken (STRING_TOKEN (STRING_FUNCTION_NINE), gHiiHandle);
  gSetupDefault         = GetToken (STRING_TOKEN (STRING_SETUP_DEFAULT), gHiiHandle);
  gEsc                  = GetToken (STRING_TOKEN (STRING_ESC), gHiiHandle);
  gEscString            = GetToken (STRING_TOKEN (STRING_ESC_STRING), gHiiHandle);
  gSelectMenu           = GetToken (STRING_TOKEN (STRING_SELECT_MENU), gHiiHandle);
  gEnter                = GetToken (STRING_TOKEN (STRING_ENTER), gHiiHandle);
  gFunctionTen          = GetToken (STRING_TOKEN (STRING_FUNCTION_TEN), gHiiHandle);
  gSaveandExit          = GetToken (STRING_TOKEN (STRING_SAVE_AND_EXIT), gHiiHandle);
  gHelpString           = GetToken (STRING_TOKEN (STRING_HELP_MESSAGE), gHiiHandle);
  gAreYouSureExit       = GetToken (STRING_TOKEN (ARE_YOU_SURE_EXIT), gHiiHandle);
  gAreYouSureExitWithSaving    = GetToken (STRING_TOKEN (ARE_YOU_SURE_EXIT_WITHSAVING), gHiiHandle);
  gAreYouSureSaveChanges    = GetToken (STRING_TOKEN (ARE_YOU_SURE_SAVING_CHANGES), gHiiHandle);
  gAreYouSureExitWithOutSaving = GetToken (STRING_TOKEN (ARE_YOU_SURE_EXIT_WITHOUTSAVING), gHiiHandle);
  gAreYouSureDiscardChanges = GetToken (STRING_TOKEN (ARE_YOU_SURE_DISCARD), gHiiHandle);
  gAreYouSureLoadDefault       = GetToken (STRING_TOKEN (ARE_YOU_SURE_LOAD_DEFAULT), gHiiHandle);
  gAreYouSureSaveUserDefault       = GetToken (STRING_TOKEN (ARE_YOU_SURE_SAVE_USER_DEFAULT), gHiiHandle);
  gAreYouSureLoadUserDefault       = GetToken (STRING_TOKEN (ARE_YOU_SURE_LOAD_USER_DEFAULT), gHiiHandle);
  gPoweronPassword      = GetToken (STRING_TOKEN (STRING_POWERON_PASSWORD), gHiiHandle);
  gSetupNotice          = GetToken (STRING_TOKEN (STRING_SETUP_NOTICE), gHiiHandle);
  gConfirmSuccess       = GetToken (STRING_TOKEN (STRING_CONFIRM_SUCCESS), gHiiHandle);
  gGeneralHelp          = GetToken (STRING_TOKEN (STRING_GENERAL_HELP), gHiiHandle);
  gGeneralHelp1         = GetToken (STRING_TOKEN (STRING_GENERAL_HELP1), gHiiHandle);
  gGeneralHelp2         = GetToken (STRING_TOKEN (STRING_GENERAL_HELP2), gHiiHandle);
  gGeneralHelp3         = GetToken (STRING_TOKEN (STRING_GENERAL_HELP3), gHiiHandle);
  gTab                  = GetToken (STRING_TOKEN (STRING_TAB_KEY), gHiiHandle);
  gBootMenuPrompt       = GetToken (STRING_TOKEN (STRING_BOOT_MENU_PROMPT), gHiiHandle);
  gBootMenuHelp1        = GetToken (STRING_TOKEN (STRING_BOOT_MENU_HELP_1), gHiiHandle);
  gBootMenuHelp2        = GetToken (STRING_TOKEN (STRING_BOOT_MENU_HELP_2), gHiiHandle);
  gBootMenuHelp3        = GetToken (STRING_TOKEN (STRING_BOOT_MENU_HELP_3), gHiiHandle);
  gFunctionOne          = GetToken (STRING_TOKEN (FUNCTION_ONE_NAME), gHiiHandle);
  gFunctionOneString    = GetToken (STRING_TOKEN (FUNCTION_ONE_STRING), gHiiHandle);
  gFunctionNineString   = GetToken (STRING_TOKEN (FUNCTION_NINE_STRING), gHiiHandle);
  gFunctionTenString    = GetToken (STRING_TOKEN (FUNCTION_TEN_STRING), gHiiHandle);
  gSaveFailed           = GetToken (STRING_TOKEN (SAVE_FAILED), gHiiHandle);
  gResultSuccess        = GetToken (STRING_TOKEN (STR_RESULT_SUCCESS), gHiiHandle);
  gResultFailed         = GetToken (STRING_TOKEN (STR_RESULT_FAILED), gHiiHandle);

  return ;
}

#define SAFE_FREE_POOL(A)    if ((A)) {FreePool ((A));(A) = NULL;}

/**
  Free up the resource allocated for all strings required
  by Setup Browser.

**/
VOID
FreeBrowserStrings (
  VOID
  )
{
  SAFE_FREE_POOL (gEnterString);
  SAFE_FREE_POOL (gEnterCommitString);
  SAFE_FREE_POOL (gEnterEscapeString);
  SAFE_FREE_POOL (gEscapeString);
  SAFE_FREE_POOL (gMoveHighlight);
  SAFE_FREE_POOL (gMakeSelection);
  SAFE_FREE_POOL (gDecNumericInput);
  SAFE_FREE_POOL (gHexNumericInput);
  SAFE_FREE_POOL (gToggleCheckBox);
  SAFE_FREE_POOL (gPromptForData);
  SAFE_FREE_POOL (gPromptForPassword);
  SAFE_FREE_POOL (gPromptForNewPassword);
  SAFE_FREE_POOL (gConfirmPassword);
  SAFE_FREE_POOL (gPassowordInvalid);
  SAFE_FREE_POOL (gConfirmError);
  SAFE_FREE_POOL (gPressEnter);
  SAFE_FREE_POOL (gEmptyString);
  SAFE_FREE_POOL (gAreYouSure);
  SAFE_FREE_POOL (gYesResponse);
  SAFE_FREE_POOL (gNoResponse);
  SAFE_FREE_POOL (gMiniString);
  SAFE_FREE_POOL (gPlusString);
  SAFE_FREE_POOL (gMinusString);
  SAFE_FREE_POOL (gAdjustNumber);
  SAFE_FREE_POOL (gSaveChanges);
  SAFE_FREE_POOL (gOptionMismatch);
  SAFE_FREE_POOL (gFormSuppress);
  SAFE_FREE_POOL (gFunctionNineString);
  SAFE_FREE_POOL (gFunctionTenString);
  SAFE_FREE_POOL (gResultFailed);
  SAFE_FREE_POOL (gResultSuccess);

  return ;
}

/**
  Show all registered HotKey help strings on bottom Rows.

**/
VOID
PrintHotKeyHelpString (
  VOID
  )
{
  UINTN                  CurrentCol;
  UINTN                  CurrentRow;
  UINTN                  BottomRowOfHotKeyHelp;
  UINTN                  ColumnWidth;
  UINTN                  Index;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  LIST_ENTRY             *Link;
  BROWSER_HOT_KEY        *HotKey;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
  ColumnWidth            = (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3;
  BottomRowOfHotKeyHelp  = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 3;

  //
  // Calculate total number of Register HotKeys. 
  //
  Index = 0;
  Link  = GetFirstNode (&gBrowserHotKeyList);
  while (!IsNull (&gBrowserHotKeyList, Link)) {
    HotKey = BROWSER_HOT_KEY_FROM_LINK (Link);
    //
    // Help string can't exceed ColumnWidth. One Row will show three Help information. 
    //
    if (StrLen (HotKey->HelpString) > ColumnWidth) {
      HotKey->HelpString[ColumnWidth] = L'\0';
    }
    //
    // Calculate help information Column and Row.
    //
    if ((Index % 3) != 2) {
      CurrentCol = LocalScreen.LeftColumn + (2 - Index % 3) * ColumnWidth;
    } else {
      CurrentCol = LocalScreen.LeftColumn + 2;
    }
    CurrentRow = BottomRowOfHotKeyHelp - Index / 3;
    //
    // Print HotKey help string on bottom Row.
    //
    PrintStringAt (CurrentCol, CurrentRow, HotKey->HelpString);

    //
    // Get Next Hot Key.
    //
    Link = GetNextNode (&gBrowserHotKeyList, Link);
    Index ++;
  }
  
  return;
}

/**
  Update key's help imformation.

  @param Selection       Tell setup browser the information about the Selection
  @param  MenuOption     The Menu option
  @param  Selected       Whether or not a tag be selected

**/
VOID
UpdateKeyHelp (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected
  )
{
  UINTN                  SecCol;
  UINTN                  ThdCol;
  UINTN                  LeftColumnOfHelp;
  UINTN                  RightColumnOfHelp;
  UINTN                  TopRowOfHelp;
  UINTN                  BottomRowOfHelp;
  UINTN                  StartColumnOfHelp;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  FORM_BROWSER_STATEMENT *Statement;

  gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_TEXT | KEYHELP_BACKGROUND);

  if (Selection->Form->ModalForm) {
    return;
  }

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  SecCol            = LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3;
  ThdCol            = LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3 * 2;

  StartColumnOfHelp = LocalScreen.LeftColumn + 2;
  LeftColumnOfHelp  = LocalScreen.LeftColumn + 1;
  RightColumnOfHelp = LocalScreen.RightColumn - 2;
  TopRowOfHelp      = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - gFooterHeight + 1;
  BottomRowOfHelp   = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 2;

  Statement = MenuOption->ThisTag;
  switch (Statement->Operand) {
  case EFI_IFR_ORDERED_LIST_OP:
  case EFI_IFR_ONE_OF_OP:
  case EFI_IFR_NUMERIC_OP:
  case EFI_IFR_TIME_OP:
  case EFI_IFR_DATE_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (!Selected) {
      //
      // On system setting, HotKey will show on every form.
      //
      if (gBrowserSettingScope == SystemLevel ||
          (Selection->FormEditable && gFunctionKeySetting != NONE_FUNCTION_KEY_SETTING)) {
        PrintHotKeyHelpString ();
      }

      if ((gClassOfVfr & FORMSET_CLASS_PLATFORM_SETUP) == FORMSET_CLASS_PLATFORM_SETUP) {
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }

      if ((Statement->Operand == EFI_IFR_DATE_OP) ||
          (Statement->Operand == EFI_IFR_TIME_OP)) {
        PrintAt (
          StartColumnOfHelp,
          BottomRowOfHelp,
          L"%c%c%c%c%s",
          ARROW_UP,
          ARROW_DOWN,
          ARROW_RIGHT,
          ARROW_LEFT,
          gMoveHighlight
          );
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gAdjustNumber);
      } else {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
        if (Statement->Operand == EFI_IFR_NUMERIC_OP && Statement->Step != 0) {
          PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gAdjustNumber);
        } 
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
      }
    } else {
      PrintStringAt (SecCol, BottomRowOfHelp, gEnterCommitString);

      //
      // If it is a selected numeric with manual input, display different message
      //
      if ((Statement->Operand == EFI_IFR_NUMERIC_OP) || 
          (Statement->Operand == EFI_IFR_DATE_OP) ||
          (Statement->Operand == EFI_IFR_TIME_OP)) {
        PrintStringAt (
          SecCol,
          TopRowOfHelp,
          ((Statement->Flags & EFI_IFR_DISPLAY_UINT_HEX) == EFI_IFR_DISPLAY_UINT_HEX) ? gHexNumericInput : gDecNumericInput
          );
      } else if (Statement->Operand != EFI_IFR_ORDERED_LIST_OP) {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      }

      if (Statement->Operand == EFI_IFR_ORDERED_LIST_OP) {
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gPlusString);
        PrintStringAt (ThdCol, TopRowOfHelp, gMinusString);
      }

      PrintStringAt (ThdCol, BottomRowOfHelp, gEnterEscapeString);
    }
    break;

  case EFI_IFR_CHECKBOX_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    //
    // On system setting, HotKey will show on every form.
    //
    if (gBrowserSettingScope == SystemLevel ||
        (Selection->FormEditable && gFunctionKeySetting != NONE_FUNCTION_KEY_SETTING)) {
      PrintHotKeyHelpString ();
    }
    if ((gClassOfVfr & FORMSET_CLASS_PLATFORM_SETUP) == FORMSET_CLASS_PLATFORM_SETUP) {
      PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
    }

    PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
    PrintStringAt (SecCol, BottomRowOfHelp, gToggleCheckBox);
    break;

  case EFI_IFR_REF_OP:
  case EFI_IFR_PASSWORD_OP:
  case EFI_IFR_STRING_OP:
  case EFI_IFR_TEXT_OP:
  case EFI_IFR_ACTION_OP:
  case EFI_IFR_RESET_BUTTON_OP:
  case EFI_IFR_SUBTITLE_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (!Selected) {
      //
      // On system setting, HotKey will show on every form.
      //
      if (gBrowserSettingScope == SystemLevel ||
          (Selection->FormEditable && gFunctionKeySetting != NONE_FUNCTION_KEY_SETTING)) {
        PrintHotKeyHelpString ();
      }
      if ((gClassOfVfr & FORMSET_CLASS_PLATFORM_SETUP) == FORMSET_CLASS_PLATFORM_SETUP) {
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }

      PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      if (Statement->Operand != EFI_IFR_TEXT_OP && Statement->Operand != EFI_IFR_SUBTITLE_OP) {
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
      }
    } else {
      if (Statement->Operand != EFI_IFR_REF_OP) {
        PrintStringAt (
          (LocalScreen.RightColumn - GetStringWidth (gEnterCommitString) / 2) / 2,
          BottomRowOfHelp,
          gEnterCommitString
          );
        PrintStringAt (ThdCol, BottomRowOfHelp, gEnterEscapeString);
      }
    }
    break;

  default:
    break;
  }
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param PackageType  Package type of the notification.

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID from the Guid
                      field of EFI_HII_PACKAGE_GUID_HEADER.
                      Otherwise, it must be NULL.

  @param Package  Points to the package referred to by the
                  notification Handle The handle of the package
                  list which contains the specified package.

  @param Handle       The HII handle.

  @param NotifyType   The type of change concerning the
                      database. See
                      EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
EFIAPI
FormUpdateNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  mHiiPackageListUpdated = TRUE;
  mReloadFormset = TRUE;

  return EFI_SUCCESS;
}

/**
  check whether the formset need to update the NV.

  @param  FormSet                FormSet data structure.

  @retval TRUE                   Need to update the NV.
  @retval FALSE                  No need to update the NV.
**/
BOOLEAN 
IsNvUpdateRequired (
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORM       *Form;

  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);

    if (Form->NvUpdateRequired ) {
      return TRUE;
    }

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }

  return FALSE;
}

/**
  check whether the formset need to update the NV.

  @param  FormSet                FormSet data structure.
  @param  SetValue               Whether set new value or clear old value.

**/
VOID
UpdateNvInfoInForm (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN BOOLEAN               SetValue
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORM       *Form;
  
  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);

    Form->NvUpdateRequired = SetValue;

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }
}

/**
  Update All formset NV info.
  
**/
VOID
UpdateAllFormsetNvInfo (
  IN BOOLEAN               SetValue
  )
{
  LIST_ENTRY           *Link;
  FORM_BROWSER_FORMSET *FormSet;
  
  Link = GetFirstNode (&gBrowserFormSetList);
  while (!IsNull (&gBrowserFormSetList, Link)) {
    FormSet = NULL;
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (Link);
    if (NULL != FormSet) {
      UpdateNvInfoInForm(FormSet, SetValue);
    }
    
    Link = GetNextNode (&gBrowserFormSetList, Link);
  }

  return;
}

/**
  Find menu which will show next time.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.
  @param Repaint         Whether need to repaint the menu.
  @param NewLine         Whether need to show at new line.
  
  @retval TRUE           Need return.
  @retval FALSE          No need to return.
**/
BOOLEAN
FindNextMenu (
  IN OUT UI_MENU_SELECTION    *Selection,
  IN     BOOLEAN              *Repaint, 
  IN     BOOLEAN              *NewLine  
  )
{
  UI_MENU_LIST            *CurrentMenu;
  CHAR16                  YesResponse;
  CHAR16                  NoResponse;
  EFI_INPUT_KEY           Key;
  BROWSER_SETTING_SCOPE   Scope;
  BYO_BROWSER_FORMSET    *ByoFormSet = NULL;

  CurrentMenu = Selection->CurrentMenu;
  mParentFormid = 0;

  if (CurrentMenu != NULL && CurrentMenu->Parent != NULL) {
    //
    // we have a parent, so go to the parent menu
    //
    if (CompareGuid (&CurrentMenu->FormSetGuid, &CurrentMenu->Parent->FormSetGuid)) {
      //
      // The parent menu and current menu are in the same formset
      //
      Selection->Action = UI_ACTION_REFRESH_FORM;
      Scope             = FormLevel;
    } else {
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      CopyMem (&Selection->FormSetGuid, &CurrentMenu->Parent->FormSetGuid, sizeof (EFI_GUID));
      Selection->Handle = CurrentMenu->Parent->HiiHandle;
      Scope             = FormSetLevel;
    }

    //
    // Form Level Check whether the data is changed.
    //
    if ((gBrowserSettingScope == FormLevel && Selection->Form->NvUpdateRequired) ||
        (gBrowserSettingScope == FormSetLevel && IsNvUpdateRequired(Selection->FormSet) && Scope == FormSetLevel)) {
      gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  
      YesResponse = gYesResponse[0];
      NoResponse  = gNoResponse[0];
  
      //
      // If NV flag is up, prompt user
      //
      do {
        CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gSaveChanges, gAreYouSure, gEmptyString);
      } while
      (
        (Key.ScanCode != SCAN_ESC) &&
        ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (NoResponse | UPPER_LOWER_CASE_OFFSET)) &&
        ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (YesResponse | UPPER_LOWER_CASE_OFFSET))
      );
  
      if (Key.ScanCode == SCAN_ESC) {
        //
        // User hits the ESC key, Ingore. 
        //
        if (Repaint != NULL) {
          *Repaint = TRUE;
        }
        if (NewLine != NULL) {
          *NewLine = TRUE;
        }

        Selection->Action = UI_ACTION_NONE;
        return FALSE;
      }
  
      if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (YesResponse | UPPER_LOWER_CASE_OFFSET)) {
        //
        // If the user hits the YesResponse key
        //
        SubmitForm (Selection->FormSet, Selection->Form, Scope);
      } else {
        //
        // If the user hits the NoResponse key
        //
        DiscardForm (Selection->FormSet, Selection->Form, Scope);
      }
    }

    Selection->Statement = NULL;

    Selection->FormId = CurrentMenu->Parent->FormId;
    Selection->QuestionId = CurrentMenu->Parent->QuestionId;

    if (!IsByoMainFormset(CurrentMenu->HiiHandle)) {
      mParentFormid = Selection->FormId;
    }
    //
    // Clear highlight record for this menu
    //
    CurrentMenu->QuestionId = 0;
    return FALSE;
  }

  if ((gClassOfVfr & FORMSET_CLASS_FRONT_PAGE) == FORMSET_CLASS_FRONT_PAGE) {
    //
    // We never exit FrontPage, so skip the ESC
    //
    Selection->Action = UI_ACTION_NONE;
    return FALSE;
  }

  //
  // We are going to leave current FormSet, so check uncommited data in this FormSet
  //
  if (gBrowserSettingScope != SystemLevel && IsNvUpdateRequired(Selection->FormSet)) {
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    YesResponse = gYesResponse[0];
    NoResponse  = gNoResponse[0];

    //
    // If NV flag is up, prompt user
    //
    do {
      CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gSaveChanges, gAreYouSure, gEmptyString);
    } while
    (
      (Key.ScanCode != SCAN_ESC) &&
      ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (NoResponse | UPPER_LOWER_CASE_OFFSET)) &&
      ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (YesResponse | UPPER_LOWER_CASE_OFFSET))
    );

    if (Key.ScanCode == SCAN_ESC) {
      //
      // User hits the ESC key
      //
      if (Repaint != NULL) {
        *Repaint = TRUE;
      }

      if (NewLine != NULL) {
        *NewLine = TRUE;
      }

      Selection->Action = UI_ACTION_NONE;
      return FALSE;
    }

    if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (YesResponse | UPPER_LOWER_CASE_OFFSET)) {
      //
      // If the user hits the YesResponse key
      //
      SubmitForm (Selection->FormSet, Selection->Form, FormSetLevel);
    } else {
      //
      // If the user hits the NoResponse key
      //
      DiscardForm (Selection->FormSet, Selection->Form, FormSetLevel);
    }
  }

  Selection->Statement = NULL;
  if (CurrentMenu != NULL) {
    CurrentMenu->QuestionId = 0;
  }

  if (bGotoGuidFormset){
    bGotoGuidFormset = FALSE;
    if (NULL != gCurrentFormSetLink) {
      ByoFormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (gCurrentFormSetLink);
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      Selection->Statement = NULL;
      Selection->Handle = FormSetGuidToHiiHandle(&ByoFormSet->Guid);
      CopyMem (&Selection->FormSetGuid, &ByoFormSet->Guid, sizeof (EFI_GUID));
    }
  } else {
    Selection->Action = UI_ACTION_EXIT;
  }
  
  return TRUE;
}

/**
  Call the call back function for the question and process the return action.

  @param Selection             On input, Selection tell setup browser the information
                               about the Selection, form and formset to be displayed.
                               On output, Selection return the screen item that is selected
                               by user.
  @param Question              The Question which need to call.
  @param Action                The action request.
  @param SkipSaveOrDiscard     Whether skip save or discard action.

  @retval EFI_SUCCESS          The call back function excutes successfully.
  @return Other value if the call back function failed to excute.  
**/
EFI_STATUS 
ProcessCallBackFunction (
  IN OUT UI_MENU_SELECTION               *Selection,
  IN     FORM_BROWSER_STATEMENT          *Question,
  IN     EFI_BROWSER_ACTION              Action,
  IN     BOOLEAN                         SkipSaveOrDiscard
  )
{
  EFI_STATUS                      Status;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_HII_VALUE                   *HiiValue;
  EFI_IFR_TYPE_VALUE              *TypeValue;
  FORM_BROWSER_STATEMENT          *Statement;
  BOOLEAN                         SubmitFormIsRequired;
  BOOLEAN                         DiscardFormIsRequired;
  BOOLEAN                         NeedExit;
  LIST_ENTRY                      *Link;
  BROWSER_SETTING_SCOPE           SettingLevel;

  ConfigAccess = Selection->FormSet->ConfigAccess;
  SubmitFormIsRequired  = FALSE;
  SettingLevel          = FormSetLevel;
  DiscardFormIsRequired = FALSE;
  NeedExit              = FALSE;
  Status                = EFI_SUCCESS;
  ActionRequest         = EFI_BROWSER_ACTION_REQUEST_NONE;

  if (ConfigAccess == NULL) {
    return EFI_SUCCESS;
  }

  Link = GetFirstNode (&Selection->Form->StatementListHead);
  while (!IsNull (&Selection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&Selection->Form->StatementListHead, Link);

    //
    // if Question != NULL, only process the question. Else, process all question in this form.
    //
    if ((Question != NULL) && (Statement != Question)) {
      continue;
    }
    
    if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != EFI_IFR_FLAG_CALLBACK) {
      continue;
    }

    //
    // Check whether Statement is disabled.
    //
    if (Statement->Expression != NULL) {
      if (EvaluateExpressionList(Statement->Expression, TRUE, Selection->FormSet, Selection->Form) == ExpressDisable) {
        continue;
      }
    }

    HiiValue = &Statement->HiiValue;
    TypeValue = &HiiValue->Value;
    if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
      //
      // For OrderedList, passing in the value buffer to Callback()
      //
      TypeValue = (EFI_IFR_TYPE_VALUE *) Statement->BufferValue;
    }
      
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = ConfigAccess->Callback (
                             ConfigAccess,
                             Action,
                             Statement->QuestionId,
                             HiiValue->Type,
                             TypeValue,
                             &ActionRequest
                             );
    if (!EFI_ERROR (Status)) {
      //
      // Only for EFI_BROWSER_ACTION_CHANGED need to handle this ActionRequest.
      //
      if (Action == EFI_BROWSER_ACTION_CHANGED) {
        switch (ActionRequest) {
        case EFI_BROWSER_ACTION_REQUEST_RESET:
          gResetRequired = TRUE;
          Selection->Action = UI_ACTION_EXIT;
          break;

        case EFI_BROWSER_ACTION_REQUEST_SUBMIT:
          SubmitFormIsRequired = TRUE;
          Selection->Action = UI_ACTION_EXIT;
          break;

        case EFI_BROWSER_ACTION_REQUEST_EXIT:
          Selection->Action = UI_ACTION_EXIT;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT:
          SubmitFormIsRequired  = TRUE;
          SettingLevel          = FormLevel;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT:
          DiscardFormIsRequired = TRUE;
          SettingLevel          = FormLevel;      
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_APPLY:
          SubmitFormIsRequired  = TRUE;
          SettingLevel          = FormLevel;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD:
          DiscardFormIsRequired = TRUE;
          SettingLevel          = FormLevel;
          break;

        default:
          break;
        }
      }

      //
      // According the spec, return value from call back of "changing" and 
      // "retrieve" should update to the question's temp buffer.
      //
      if (Action == EFI_BROWSER_ACTION_CHANGING || Action == EFI_BROWSER_ACTION_RETRIEVE) {
        SetQuestionValue(Selection->FormSet, Selection->Form, Statement, GetSetValueWithEditBuffer);
      }
    } else {
      //
      // According the spec, return fail from call back of "changing" and 
      // "retrieve", should restore the question's value.
      //
      if (Action  == EFI_BROWSER_ACTION_CHANGING || Action == EFI_BROWSER_ACTION_RETRIEVE) {
        GetQuestionValue(Selection->FormSet, Selection->Form, Statement, GetSetValueWithEditBuffer);
      }

      if (Status == EFI_UNSUPPORTED) {
        //
        // If return EFI_UNSUPPORTED, also consider Hii driver suceess deal with it.
        //
        Status = EFI_SUCCESS;
      }
    }
  }

  if (SubmitFormIsRequired && !SkipSaveOrDiscard) {
    SubmitForm (Selection->FormSet, Selection->Form, SettingLevel);
  }

  if (DiscardFormIsRequired && !SkipSaveOrDiscard) {
    DiscardForm (Selection->FormSet, Selection->Form, SettingLevel);
  }

  if (NeedExit) {
    FindNextMenu (Selection, NULL, NULL);
  }

  return Status;
}

/**
  Call the retrieve type call back function for one question to get the initialize data.
  
  This function only used when in the initialize stage, because in this stage, the 
  Selection->Form is not ready. For other case, use the ProcessCallBackFunction instead.

  @param ConfigAccess          The config access protocol produced by the hii driver.
  @param Statement             The Question which need to call.

  @retval EFI_SUCCESS          The call back function excutes successfully.
  @return Other value if the call back function failed to excute.  
**/
EFI_STATUS 
ProcessRetrieveForQuestion (
  IN     EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess,
  IN     FORM_BROWSER_STATEMENT          *Statement
  )
{
  EFI_STATUS                      Status;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_HII_VALUE                   *HiiValue;
  EFI_IFR_TYPE_VALUE              *TypeValue;

  Status                = EFI_SUCCESS;
  ActionRequest         = EFI_BROWSER_ACTION_REQUEST_NONE;
    
  if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != EFI_IFR_FLAG_CALLBACK) {
    return EFI_UNSUPPORTED;
  }

  HiiValue  = &Statement->HiiValue;
  TypeValue = &HiiValue->Value;
  if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
    //
    // For OrderedList, passing in the value buffer to Callback()
    //
    TypeValue = (EFI_IFR_TYPE_VALUE *) Statement->BufferValue;
  }
    
  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = ConfigAccess->Callback (
                           ConfigAccess,
                           EFI_BROWSER_ACTION_RETRIEVE,
                           Statement->QuestionId,
                           HiiValue->Type,
                           TypeValue,
                           &ActionRequest
                           );
  return Status;
}

/**
  The worker function that send the displays to the screen. On output,
  the selection made by user is returned.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.

  @retval EFI_SUCCESS    The page is displayed successfully.
  @return Other value if the page failed to be diplayed.

**/
EFI_STATUS
SetupBrowser (
  IN OUT UI_MENU_SELECTION    *Selection
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  EFI_HANDLE                      NotifyHandle;
  FORM_BROWSER_STATEMENT          *Statement;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_INPUT_KEY                   Key;

  gMenuRefreshHead = NULL;
  ConfigAccess = Selection->FormSet->ConfigAccess;
  //
  // Register notify for Form package update
  //
  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORMS,
                           NULL,
                           FormUpdateNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &NotifyHandle
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  if ((NULL == GetFormSetFromHiiHandle (Selection->Handle)) ||
    (TRUE == mReloadFormset)) {
    //
    // Initialize current settings of Questions in this FormSet
    //
    Status = InitializeCurrentSetting (Selection->FormSet);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    if (NULL == GetFormSetFromHiiHandle (Selection->Handle)) {
      Selection->FormId = gFirstFormId;
    }
    //
    // Remove old FormSet data when the Form Package is updated.
    //
    if ((TRUE == mReloadFormset) || (NULL == GetFormSetFromHiiHandle (Selection->Handle) &&  !IsByoMainFormset(Selection->Handle))) {
      if (NULL != gOldFormSet) {
        RemoveEntryList (&gOldFormSet->Link);
        DestroyFormSet (gOldFormSet);
        gOldFormSet = NULL;
      }
      mReloadFormset = FALSE;
    }
    //
    // Update gOldFormSet on maintain back up FormSet list.
    // And, make gOldFormSet point to current FormSet.
    //
    InsertTailList (&gBrowserFormSetList, &Selection->FormSet->Link);
    RefreshFormsetVariable(Selection->FormSet);
  } else {
    Selection->FormSet = GetFormSetFromHiiHandle (Selection->Handle);
    Selection->FormId = gFirstFormId; 
    if (0 != mParentFormid) {
      Selection->FormId = mParentFormid;
      mParentFormid = 0;
    }
  }

  gOldFormSet = Selection->FormSet;

  do {
    //
    // Check whether Form Package has been updated during Callback
    //
    if (mHiiPackageListUpdated) {
      //
      // Force to reparse IFR binary of target Formset
      //
      mHiiPackageListUpdated = FALSE;
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      break;
    }
    //
    // Initialize Selection->Form.
    //
    if (Selection->FormId == gFirstFormId) {
      //
      // "1" FormId indicates display the first Form in a FormSet
      //
      Link = GetFirstNode (&Selection->FormSet->FormListHead);
      Selection->Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      Selection->FormId = Selection->Form->FormId;
    } else {
      Selection->Form = IdToForm (Selection->FormSet, Selection->FormId);
    }

    if (Selection->Form == NULL) {
      //
      // No Form to display
      //
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    //
    // Check Form is suppressed.
    //
    if (Selection->Form->SuppressExpression != NULL) {
      if (EvaluateExpressionList(Selection->Form->SuppressExpression, TRUE, Selection->FormSet, Selection->Form) == ExpressSuppress) {
        //
        // Form is suppressed. 
        //
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gFormSuppress, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        Status = EFI_NOT_FOUND;
        goto Done;
      }
    }
    //
    // Before display new form, invoke ConfigAccess.Callback() with EFI_BROWSER_ACTION_FORM_OPEN
    // for each question with callback flag.
    // New form may be the first form, or the different form after another form close.
    //
    if ((ConfigAccess != NULL) &&
        ((Selection->Handle != mCurrentHiiHandle) ||
        (!CompareGuid (&Selection->FormSetGuid, &mCurrentFormSetGuid)) ||
        (Selection->FormId != mCurrentFormId))) {

      //
      // Keep current form information
      //
      mCurrentHiiHandle   = Selection->Handle;
      CopyGuid (&mCurrentFormSetGuid, &Selection->FormSetGuid);
      mCurrentFormId      = Selection->FormId;

      Status = ProcessCallBackFunction (Selection, NULL, EFI_BROWSER_ACTION_FORM_OPEN, FALSE);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // EXIT requests to close form.
      //
      if (Selection->Action == UI_ACTION_EXIT) {
        goto Done;
      }
      //
      // IFR is updated during callback of open form, force to reparse the IFR binary
      //
      if (mHiiPackageListUpdated) {
        Selection->Action = UI_ACTION_REFRESH_FORMSET;
        mHiiPackageListUpdated = FALSE;
        break;
      }
    }

    //
    // Load Questions' Value for display
    //
    Status = LoadFormSetConfig (Selection, Selection->FormSet);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // EXIT requests to close form.
    //
    if (Selection->Action == UI_ACTION_EXIT) {
      goto Done;
    }
    //
    // IFR is updated during callback of read value, force to reparse the IFR binary
    //
    if (mHiiPackageListUpdated) {
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      mHiiPackageListUpdated = FALSE;
      break;
    }

    //
    // Displays the Header and Footer borders
    //
    DisplayPageFrame (Selection);

    //
    // Display form
    //
    Status = DisplayForm (Selection);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Check Selected Statement (if press ESC, Selection->Statement will be NULL)
    //
    Statement = Selection->Statement;
    if (Statement != NULL) {
      if ((Statement->QuestionFlags & EFI_IFR_FLAG_RESET_REQUIRED) == EFI_IFR_FLAG_RESET_REQUIRED) {
        gResetRequired = TRUE;
      }

      if ((ConfigAccess != NULL) && 
          ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) == EFI_IFR_FLAG_CALLBACK) && 
          (Statement->Operand != EFI_IFR_PASSWORD_OP)) {

        Status = ProcessCallBackFunction(Selection, Statement, EFI_BROWSER_ACTION_CHANGING, FALSE);         
        if (Statement->Operand == EFI_IFR_REF_OP && Selection->Action != UI_ACTION_EXIT) {
          //
          // Process dynamic update ref opcode.
          //
          if (!EFI_ERROR (Status)) {
            Status = ProcessGotoOpCode(Statement, Selection, NULL, NULL);
          }
          
          //
          // Callback return error status or status return from process goto opcode.
          //
          if (EFI_ERROR (Status)) {
            //
            // Cross reference will not be taken
            //
            Selection->FormId = Selection->Form->FormId;
            Selection->QuestionId = 0;
          }
        }

        if (!EFI_ERROR (Status) && Statement->Operand != EFI_IFR_REF_OP) {
          ProcessCallBackFunction(Selection, Statement, EFI_BROWSER_ACTION_CHANGED, FALSE);
        }
      }

      //
      // Check whether Form Package has been updated during Callback
      //
      if (gExitRequired) {
        switch (gBrowserSettingScope) {
        case SystemLevel:
        case FormSetLevel:
          Selection->Action = UI_ACTION_EXIT;
          break;

        case FormLevel:
          FindNextMenu (Selection, NULL, NULL);
          break;

        default:
          break;
        }

        gExitRequired = FALSE;
      }
    }

    //
    // Before exit the form, invoke ConfigAccess.Callback() with EFI_BROWSER_ACTION_FORM_CLOSE
    // for each question with callback flag.
    //
    if ((ConfigAccess != NULL) && 
        ((Selection->Action == UI_ACTION_EXIT) || 
         (Selection->Handle != mCurrentHiiHandle) ||
         (!CompareGuid (&Selection->FormSetGuid, &mCurrentFormSetGuid)) ||
         (Selection->FormId != mCurrentFormId))) {

      Status = ProcessCallBackFunction (Selection, NULL, EFI_BROWSER_ACTION_FORM_CLOSE, FALSE);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
  } while (Selection->Action == UI_ACTION_REFRESH_FORM);

Done:
  //
  // Reset current form information to the initial setting when error happens or form exit.
  //
  if (EFI_ERROR (Status) || Selection->Action == UI_ACTION_EXIT) {
    mCurrentHiiHandle = NULL;
    CopyGuid (&mCurrentFormSetGuid, &gZeroGuid);
    mCurrentFormId = 1;
  }

  //
  // Unregister notify for Form package update
  //
  mHiiDatabase->UnregisterPackageNotify (
                   mHiiDatabase,
                   NotifyHandle
                   );
  return Status;
}

/**
  Draw Boot Manager Menu Frame.
**/
VOID
DrawBootMenuFrame (
  VOID
  )
{
  UINTN                    Index;
  CHAR16                   *Buffer;
  UINTN                    Row;
  UINTN                    TopRow;
  UINTN                    BottomRow;
  UINTN                    LeftColumn;
  UINTN                    RightColumn;
  EFI_SCREEN_DESCRIPTOR    LocalScreen;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  LeftColumn  = (LocalScreen.RightColumn - BOOT_MENU_LENGTH)/2;
  RightColumn = LeftColumn + BOOT_MENU_LENGTH;
  TopRow      = (LocalScreen.BottomRow - BOOT_MENU_HEIGHT)/2;
  BottomRow   = TopRow + BOOT_MENU_HEIGHT;

  ClearLines (LeftColumn, RightColumn, TopRow, BottomRow, EFI_WHITE | EFI_BACKGROUND_BLUE);

  Buffer = AllocateZeroPool (0x1000);
  ASSERT(Buffer);

  for(Index = 0; Index < BOOT_MENU_LENGTH - 2; Index++){
    Buffer[Index] = BOXDRAW_HORIZONTAL;
  }

  PrintCharAt (LeftColumn, TopRow, BOXDRAW_DOWN_RIGHT);
  PrintString (Buffer);
  PrintChar (BOXDRAW_DOWN_LEFT);
  PrintCharAt (LeftColumn, TopRow + 1, BOXDRAW_VERTICAL);
  PrintCharAt (RightColumn - 1, TopRow + 1, BOXDRAW_VERTICAL);
  PrintCharAt (LeftColumn, TopRow + 2, BOXDRAW_VERTICAL_RIGHT);
  PrintString (Buffer);
  PrintChar(BOXDRAW_VERTICAL_LEFT);

  for(Row = TopRow + 3; Row < BottomRow - 4; Row ++){
    PrintCharAt (LeftColumn, Row, BOXDRAW_VERTICAL);
    PrintCharAt (RightColumn - 1, Row, BOXDRAW_VERTICAL);
  }

  PrintCharAt (LeftColumn, BottomRow - 4, BOXDRAW_VERTICAL_RIGHT);
  PrintString (Buffer);
  PrintChar(BOXDRAW_VERTICAL_LEFT);

  PrintCharAt (LeftColumn, BottomRow - 3, BOXDRAW_VERTICAL);
  PrintCharAt (RightColumn - 1, BottomRow - 3, BOXDRAW_VERTICAL);
  PrintCharAt (LeftColumn, BottomRow - 2, BOXDRAW_VERTICAL);
  PrintCharAt (RightColumn - 1, BottomRow - 2, BOXDRAW_VERTICAL);
  PrintCharAt (LeftColumn, BottomRow - 1, BOXDRAW_VERTICAL);
  PrintCharAt (RightColumn - 1, BottomRow - 1, BOXDRAW_VERTICAL);

  PrintCharAt (LeftColumn, BottomRow, BOXDRAW_UP_RIGHT);
  PrintString (Buffer);
  PrintChar(BOXDRAW_UP_LEFT);

  //Print String "Please select boot device:"
  PrintStringAt(
    (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (gBootMenuPrompt) / 2) / 2,
    TopRow + 1,
    gBootMenuPrompt
    );

  //Print Help String
  PrintAt(
    (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (gBootMenuHelp1) / 2) / 2,
    BottomRow - 3,
    L"%c%c%s",
    ARROW_UP,
    ARROW_DOWN,
    gBootMenuHelp1
    );

  PrintStringAt(
    (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (gBootMenuHelp2) / 2) / 2,
    BottomRow - 2,
    gBootMenuHelp2
    );

  PrintStringAt(
    (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (gBootMenuHelp3) / 2) / 2,
    BottomRow - 1,
    gBootMenuHelp3
    );

  gBS->FreePool (Buffer);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);

  return;
}

/**
  Draw background of platform setup.
**/
VOID
DrawSetupBackground (VOID )
{
  UINTN                  Index;
  CHAR16                *Buffer = NULL;
  CHAR16                *SecBuffer = NULL;
  UINTN                  Row;
  UINTN                  Length;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  
  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Length = LocalScreen.RightColumn - LocalScreen.LeftColumn;

  //
  // Fill background of help info to avoid scroll screen.
  //
  GopBltArea (
    LocalScreen.LeftColumn,
    LocalScreen.RightColumn,
    LocalScreen.BottomRow - 2,
    LocalScreen.BottomRow - 1,
    EFI_WHITE |EFI_BACKGROUND_BLUE
  );
  
  //
  // Repare black line.
  //
  RepairScreen ();

  Buffer = AllocateZeroPool ((gMiddleVerticalLineColumn + 1)*sizeof(CHAR16));
  ASSERT (Buffer != NULL);
  for (Index = 0; Index + 1 < gMiddleVerticalLineColumn; Index++) {
    Buffer[Index] = BOXDRAW_HORIZONTAL;
  }

  SecBuffer = AllocateZeroPool ((LocalScreen.RightColumn - gMiddleVerticalLineColumn + 1)*sizeof(CHAR16));
  ASSERT (SecBuffer != NULL);
  for (Index = 0; Index + 1 < LocalScreen.RightColumn - gMiddleVerticalLineColumn; Index++) {
    SecBuffer[Index] = BOXDRAW_HORIZONTAL;
  }

  //
  // "Byosoft BIOS Setup Utility", line 1.
  // 
  GopBltArea (
    LocalScreen.LeftColumn,
    LocalScreen.RightColumn,
    LocalScreen.TopRow,
    LocalScreen.TopRow,
    EFI_GREEN |EFI_BACKGROUND_BLUE
  );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_GREEN | EFI_BACKGROUND_BLUE);
  PrintStringAt (
    (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (gByoSoftWare) / 2) / 2,
    LocalScreen.TopRow,
    gByoSoftWare
  );

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);

  //
  // line 2.
  //
  PrintCharAt (LocalScreen.LeftColumn, LocalScreen.TopRow + 2, BOXDRAW_DOWN_RIGHT);
  PrintString (Buffer);
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.TopRow + 2, BOXDRAW_DOWN_HORIZONTAL);
  PrintString (SecBuffer);
  PrintCharAt (LocalScreen.RightColumn - 1, LocalScreen.TopRow + 2, BOXDRAW_DOWN_LEFT);

  //
  // line 3.
  //
  PrintCharAt(LocalScreen.LeftColumn, LocalScreen.TopRow + 3, BOXDRAW_VERTICAL);
  PrintCharAt(LocalScreen.RightColumn - 1, LocalScreen.TopRow + 3, BOXDRAW_VERTICAL);
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.TopRow + 3, BOXDRAW_VERTICAL);

  //
  // line 4.
  //
  PrintCharAt(LocalScreen.LeftColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL);
  PrintCharAt(gMiddleVerticalLineColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_RIGHT);
  PrintString (SecBuffer);
  PrintCharAt(LocalScreen.RightColumn - 1, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_LEFT);

  //
  // Middle, line to "LocalScreen.BottomRow - 3".
  //
  for(Row = LocalScreen.TopRow + 5; Row < LocalScreen.BottomRow - 3; Row ++){
      PrintCharAt(LocalScreen.LeftColumn, Row, BOXDRAW_VERTICAL);
      PrintCharAt(gMiddleVerticalLineColumn, Row, BOXDRAW_VERTICAL);    
      PrintCharAt(LocalScreen.RightColumn - 1, Row, BOXDRAW_VERTICAL);
  }

  //
  // line "LocalScreen.BottomRow - 3".
  //
  PrintCharAt(LocalScreen.LeftColumn,LocalScreen.BottomRow - 3,BOXDRAW_UP_RIGHT);
  PrintString (Buffer);
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.BottomRow - 3, BOXDRAW_UP_HORIZONTAL);
  PrintString (SecBuffer);
  PrintCharAt(LocalScreen.RightColumn - 1, LocalScreen.BottomRow - 3, BOXDRAW_UP_LEFT);

  //
  // "Help Message", line 3.
  //
  GopBltArea (
    LocalScreen.LeftColumn + gMiddleVerticalLineColumn +1,
    LocalScreen.RightColumn -1,
    LocalScreen.TopRow + 3,
    LocalScreen.TopRow + 3,
    EFI_BACKGROUND_LIGHTGRAY
  );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BRIGHT | FIELD_BACKGROUND);
  PrintStringAt(gMiddleVerticalLineColumn + (gHelpBlockWidth - GetStringWidth (gHelpString)/2) / 2, LocalScreen.TopRow + 3, gHelpString);

  //
  // Hotkey help information.
  //
  DrawBottomHelpInfo();

  gBS->FreePool(Buffer);
  gBS->FreePool(SecBuffer);
  return;
}
/**
  Draw the background of Help message .
**/
VOID
DrawHelpMessageBackground (VOID )
{
  UINTN                  Index;
  CHAR16                *Buffer = NULL;

  EFI_SCREEN_DESCRIPTOR  LocalScreen;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Buffer = AllocateZeroPool ((LocalScreen.RightColumn - gMiddleVerticalLineColumn + 1)*sizeof(CHAR16));
  ASSERT (Buffer != NULL);
  for (Index = 0; Index + 1 < LocalScreen.RightColumn - gMiddleVerticalLineColumn; Index++) {
    Buffer[Index] = BOXDRAW_HORIZONTAL;
  }

  //
  // line 2.
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.TopRow + 2, BOXDRAW_DOWN_HORIZONTAL);

  //
  // "Help Message", line 3.
  //
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.TopRow + 3, BOXDRAW_VERTICAL);

  GopBltArea (
    LocalScreen.LeftColumn + gMiddleVerticalLineColumn +1,
    LocalScreen.RightColumn -1,
    LocalScreen.TopRow + 3,
    LocalScreen.TopRow + 3,
    EFI_BACKGROUND_LIGHTGRAY
  );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BRIGHT | FIELD_BACKGROUND);
  PrintStringAt(gMiddleVerticalLineColumn + (gHelpBlockWidth - GetStringWidth (gHelpString)/2) / 2, LocalScreen.TopRow + 3, gHelpString);
  //
  // line 4.
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);
  PrintCharAt(gMiddleVerticalLineColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_RIGHT);
  PrintString (Buffer);
  PrintCharAt(LocalScreen.RightColumn - 1, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL_LEFT);

  //
  // line "LocalScreen.BottomRow - 3".
  //
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.BottomRow - 3, BOXDRAW_UP_HORIZONTAL);


  gBS->FreePool(Buffer);
  return;
}

/**
  Clean the background of Help message .
**/
VOID
CleanHelpMessageBackground (VOID )
{
  UINTN                  Index;
  EFI_SCREEN_DESCRIPTOR    LocalScreen;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // line 2.
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);
  PrintCharAt (gMiddleVerticalLineColumn, LocalScreen.TopRow + 2, BOXDRAW_HORIZONTAL);
  //
  // "Help Message", line 3.
  //
  GopBltArea (
    LocalScreen.LeftColumn + gMiddleVerticalLineColumn ,
    LocalScreen.RightColumn -1,
    LocalScreen.TopRow + 3,
    LocalScreen.TopRow + 3,
    EFI_BACKGROUND_LIGHTGRAY
  );
  //
  // Middle, line to "LocalScreen.BottomRow - 3".
  //
  GopBltArea (
    LocalScreen.LeftColumn + gMiddleVerticalLineColumn ,
    LocalScreen.RightColumn - 1,
    LocalScreen.TopRow + 3,
    LocalScreen.BottomRow - 4,
    EFI_BACKGROUND_LIGHTGRAY
  );
  //
  // line 4.
  //
  GopBltArea (
    LocalScreen.LeftColumn + gMiddleVerticalLineColumn ,
    LocalScreen.RightColumn - 1,
    LocalScreen.TopRow + 4,
    LocalScreen.TopRow + 4,
    EFI_BACKGROUND_LIGHTGRAY
  );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);  
  PrintCharAt(LocalScreen.LeftColumn, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL);
  PrintCharAt(LocalScreen.RightColumn - 1, LocalScreen.TopRow + 4, BOXDRAW_VERTICAL);
  //
  // line "LocalScreen.BottomRow - 3".
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BLUE | FIELD_BACKGROUND);
  for (Index = gMiddleVerticalLineColumn; Index < LocalScreen.RightColumn - 1; Index++) {
    PrintCharAt (Index, LocalScreen.BottomRow - 3, BOXDRAW_HORIZONTAL);
  }

  return;
}

/**
  Calculate Starting Menu Index from end to begin.

**/
UINTN
GetStartMenuIndex (
  IN CONST LIST_ENTRY    *FormSetList,
  IN UINTN    CurrentIndex
  )
{
  UINTN    StartItem;
  UINTN    TotalItems;
  BYO_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY    *Link;
  CHAR16    *FormsetTitle;
  UINTN    BarLength;
  UINTN    StringWidth;
  UINTN    MenuCount;

  if (FormSetList == NULL) {
    return (UINTN) -1;
  }

  BarLength = 3;
  TotalItems = 0;
  Link = GetFirstNode (FormSetList);
  while (!IsNull (FormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);

    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FormsetTitle = NULL;	      
      FormsetTitle = GetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
      if (FormsetTitle) {
        BarLength += GetStringWidth(FormsetTitle) / 2;
        BarLength ++;
        FreePool (FormsetTitle);
      }
    }
    TotalItems++;
    Link = GetNextNode (FormSetList, Link);
  }
  if (CurrentIndex > TotalItems) {
    return (UINTN) -1;
  }
  if (gScreenDimensions.RightColumn > BarLength) {
    return 0;
  }
  
  //
  // Check if have more item to show on menu bar.
  //
  StartItem = 0;
  BarLength = 3;
  MenuCount = 0;
  Link = GetPreviousNode (FormSetList, Link);
  while (!IsNull (FormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);

    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      if (CurrentIndex >= (TotalItems - MenuCount - 1)) {
      FormsetTitle = NULL;	      
      FormsetTitle = GetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
      if (FormsetTitle) {
        StringWidth =  GetStringWidth(FormsetTitle) / 2;
        if (gScreenDimensions.RightColumn < BarLength + StringWidth) {
          StartItem = TotalItems - MenuCount;
          break;
        }
        BarLength += StringWidth;
        BarLength ++;		
        FreePool (FormsetTitle);
      }
      }
    }
    MenuCount ++;
    Link = GetPreviousNode (FormSetList, Link);
  }
  
  return StartItem;
}


/**
  Draw form title.

  @param  Selection               Input Selection data structure.
  @param  bMainFormTitle       TRUE is main form title, FALSe is sub form title.

**/
VOID
DrawFormTitleBar (
  IN UI_MENU_SELECTION    *Selection,
  BOOLEAN bMainFormTitle
  )
{
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  BYO_BROWSER_FORMSET   *FormSet;
  BYO_BROWSER_FORMSET   *CurrentFormSet;
  LIST_ENTRY          *Link;
  UINTN                  CursorPos;
  UINTN                  Index;
  UINTN                  Count;
  UINTN                  TotalItems;
  UINTN                  CurrentItem;
  UINTN                  StartItem;
  UINTN                  TitleLength;
  UINTN                  StringWidth;
  CHAR16               *FormsetTitle;
  static UINTN         LastIndex = (UINTN) -1;
  static BOOLEAN    LastMainFormTitle = FALSE;
  static UINTN    LastStartItem = 0;
  static UINTN    LastEndItem = 0;


  if (LastMainFormTitle != bMainFormTitle || bDrawSetupBackground) {
    LastIndex = (UINTN) -1;
  }
  //
  // Draw background.
  // 
  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  //Ckeck gCurrentFormSetLink.
  // 
  CurrentFormSet = NULL;      
  if (NULL == gCurrentFormSetLink) {
    //
    //Draw Tilte in Middle.
    //
    FormsetTitle = NULL;
    FormsetTitle = GetToken(Selection->FormSet->FormSetTitle, Selection->Handle);
    if (NULL == FormsetTitle) {
      FormsetTitle = L"NULL";
    }

    StringWidth = GetStringWidth(FormsetTitle)/2;
    TitleLength = LocalScreen.RightColumn - LocalScreen.LeftColumn; 
    if (TitleLength >  StringWidth) { 
      CursorPos = (TitleLength - StringWidth)/2;
    } else {
      CursorPos = 4;
    }
  
    gST->ConOut->SetAttribute (gST->ConOut, MENU_TEXT_UNSEL | MENU_BACKGROUND_UNSEL); 
    PrintStringAt ( CursorPos,LocalScreen.TopRow + 1, FormsetTitle);
  
    return; 
  } else {
    CurrentFormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (gCurrentFormSetLink);
  }

  //
  // Get info of gByoFormSetList.
  //
  TotalItems = 0;
  CurrentItem = 0;  
  Link = GetFirstNode (gByoFormSetList);
  while (!IsNull (gByoFormSetList, Link)) {
    FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
    if (CompareGuid(&FormSet->Guid, &CurrentFormSet->Guid)) {
      CurrentItem = TotalItems;
    }
    TotalItems++;
    Link = GetNextNode (gByoFormSetList, Link);
  }

  //
  // Calculate start item.
  //
  StartItem = 0;
  if (CurrentItem) {
    if (LastStartItem) {
      if (CurrentItem >= LastStartItem) {
        StartItem = GetStartMenuIndex (gByoFormSetList, CurrentItem);;
      } else if (CurrentItem == 0 && LastStartItem == TotalItems -1) {
        StartItem = 0;
        LastStartItem = 0;
      }
    } else {
      StartItem = GetStartMenuIndex (gByoFormSetList, CurrentItem);
      LastStartItem = 0;
    }  
  } else {
    StartItem = 0;
    LastStartItem = 0;
  }

  //
  // Draw all menu title.
  //
  GopBltArea (LocalScreen.LeftColumn, LocalScreen.RightColumn, LocalScreen.TopRow + 1, LocalScreen.TopRow + 1, MENU_TEXT_UNSEL | MENU_BACKGROUND_UNSEL);
  
  CursorPos = 3;
  for (Index = StartItem; Index < TotalItems; Index++) {   

    //
    // Get current Formset.
    //
    Link = GetFirstNode (gByoFormSetList);
    Count = 0;
    FormSet = NULL;
    while (!IsNull (gByoFormSetList, Link)) {
      FormSet = BYO_FORM_BROWSER_FORMSET_FROM_LINK (Link);
      if (Index == Count) {
        break;
      }
      Count++;
      Link = GetNextNode (gByoFormSetList, Link);
    }
  
    FormsetTitle = NULL;
    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FormsetTitle = GetToken(FormSet->FormSetTitle, FormSet->HiiHandle);
    } else {
      FormsetTitle = L"NULL";
    }
    StringWidth = GetStringWidth(FormsetTitle)/2;
    if (LocalScreen.RightColumn < CursorPos + StringWidth) {	
      if (LastEndItem <= CurrentItem) {
        LastStartItem =  LastStartItem + 1;	
      }
      break;
    }
    if (FALSE == bMainFormTitle && CurrentItem != Index) {
      CursorPos = CursorPos + StringWidth + 1;    
      continue;
    }

    //
    // Draw Title.
    //
    if (CurrentItem == Index) {
      gST->ConOut->SetAttribute (gST->ConOut, MENU_TEXT_SEL | MENU_BACKGROUND_SEL);
      PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
      PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
      PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
    } else {
      if (LastIndex <= TotalItems) {
        gST->ConOut->SetAttribute (gST->ConOut, MENU_TEXT_UNSEL | MENU_BACKGROUND_UNSEL);
        PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
        PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
        PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, MENU_TEXT_UNSEL | MENU_BACKGROUND_UNSEL);
        PrintStringAt(CursorPos,  LocalScreen.TopRow + 1, L" ");
        PrintStringAt(CursorPos + 1,  LocalScreen.TopRow + 1, FormsetTitle);
        PrintStringAt(CursorPos + StringWidth,  LocalScreen.TopRow + 1, L" ");
      }
    } 
  
    if (0 != FormSet->FormSetTitle && NULL != FormSet->HiiHandle) {
      FreePool(FormsetTitle);
    }

    CursorPos = CursorPos + StringWidth + 1;    
  }

  LastIndex = CurrentItem;
  LastMainFormTitle = bMainFormTitle;
  LastEndItem = Index - 1;

  //
  //Show red arrow to mark the more main fromset.
  //
  if (StartItem > 0 ) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_RED| MENU_BACKGROUND_UNSEL);
    PrintCharAt (
        LocalScreen.LeftColumn,
        LocalScreen.TopRow + 1,
        GEOMETRICSHAPE_LEFT_TRIANGLE
        );
  }
  if (LastEndItem < TotalItems - 1) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_RED| MENU_BACKGROUND_UNSEL);
    PrintCharAt (
        LocalScreen.RightColumn-1,
        LocalScreen.TopRow + 1,
        GEOMETRICSHAPE_RIGHT_TRIANGLE
        );
  }

  return ;
}


/**
  If Sting length greater Max, Copy Max string to Buf.
  
**/
BOOLEAN
CutMaxString ( 
  CHAR16    **Buf,
  CHAR16    *String,
  UINTN    Max
  )
{
  if (StrLen(String) > Max) {
    ZeroMem(*Buf, (Max + 1) * sizeof(CHAR16));
    CopyMem(*Buf, String, Max * sizeof(CHAR16));
    return TRUE;    
  }
  return FALSE;  
}
/**
  Draw Help Info.
  
**/
VOID
DrawBottomHelpInfo (
  VOID
  )
{
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  UINTN    Width;
  UINTN    StartColum;
  UINTN    StartRow;
  UINTN    MaxLength;
  CHAR16    *Buf; 

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Width = (LocalScreen.RightColumn - LocalScreen.LeftColumn - 2)/4;
  MaxLength = Width -8;

  Buf = NULL; 
  Buf = AllocateZeroPool(Width * sizeof(CHAR16));
  ASSERT(NULL != Buf);
 
  StartColum = LocalScreen.LeftColumn + 2;
  StartRow = LocalScreen.BottomRow - 2;
  //
  // Print HotKey, Line at LocalScreen.BottomRow - 2.
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_GREEN|EFI_BACKGROUND_BLUE);
  PrintStringAt(StartColum, StartRow, gFunctionOne);
  PrintAt (StartColum + Width - 4, StartRow, L"%c%c", ARROW_UP, ARROW_DOWN);  
  PrintStringAt (StartColum + Width * 2 - 2, StartRow, gMinusPlus);
  PrintStringAt (StartColum + Width * 3, StartRow, gFunctionNine);

  //
  // Print HotKey, Line at LocalScreen.BottomRow - 1.
  //
  StartRow +=1;
  PrintStringAt(StartColum , StartRow, gEsc);
  PrintAt (StartColum + Width - 4, StartRow, L"%c%c", ARROW_LEFT, ARROW_RIGHT);  
  PrintStringAt (StartColum + Width * 2 - 2, StartRow, gEnter);
  PrintStringAt (StartColum + Width * 3, StartRow, gFunctionTen);

  //
  // Print Help String, Line at LocalScreen.BottomRow - 2.
  //
  StartColum += 6;
  StartRow -=1;
  gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE|EFI_BACKGROUND_BLUE);
  if (CutMaxString(&Buf, gFunctionOneString, MaxLength)) {
    PrintAt (StartColum, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum, StartRow, gFunctionOneString);
  }

  if (CutMaxString(&Buf, gSelectItem, MaxLength)) {
    PrintAt (StartColum + Width - 4, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width - 4, StartRow, gSelectItem);
  }

  if (CutMaxString(&Buf, gChangeValue, MaxLength)) {
    PrintAt (StartColum + Width * 2 - 2, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 2 - 2, StartRow, gChangeValue);
  }
  
  if (CutMaxString(&Buf, gSetupDefault, MaxLength)) {
    PrintAt (StartColum + Width * 3, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gSetupDefault);
  }

  //
  // Print Help String, Line at LocalScreen.BottomRow - 1.
  //
  StartRow +=1;
  PrintStringAt(StartColum , StartRow, gEscString);
  if (CutMaxString(&Buf, gEscString, MaxLength)) {
    PrintAt (StartColum, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum, StartRow, gEscString);
  }

  if (CutMaxString(&Buf, gSelectMenu, MaxLength)) {
    PrintAt (StartColum + Width - 4, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width - 4, StartRow, gSelectMenu);
  }

  if (CutMaxString(&Buf,  L"Enter Sub Item", MaxLength)) {
    PrintAt (StartColum + Width * 2 - 2, StartRow, Buf);    
  } else {
    PrintAt (StartColum + Width * 2 - 2, StartRow, L"Enter Sub Item");  
  }

  if (CutMaxString(&Buf, gSaveandExit, MaxLength)) {
    PrintAt (StartColum + Width * 3, StartRow, Buf);    
  } else {
    PrintStringAt (StartColum + Width * 3, StartRow, gSaveandExit);
  }
  
  return;
}
