/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Software Genlock", "index.html", [
    [ "Introduction SW Genlock", "index.html#autotoc_md15", [
      [ "Key Features of SW Genlock", "index.html#autotoc_md16", null ],
      [ "Known Issues", "index.html#autotoc_md17", null ]
    ] ],
    [ "Installation", "index.html#autotoc_md18", [
      [ "System Requirements", "index.html#autotoc_md19", null ],
      [ "Build steps", "index.html#autotoc_md20", null ]
    ] ],
    [ "Software Components", "index.html#autotoc_md21", [
      [ "Vsync Library", "index.html#autotoc_md22", null ],
      [ "Reference Applications", "index.html#autotoc_md23", null ]
    ] ],
    [ "Generating Doxygen documents", "index.html#autotoc_md24", null ],
    [ "Synchronization between two systems", "index.html#autotoc_md25", [
      [ "Synchronizing Clocks between two systems", "index.html#autotoc_md26", [
        [ "Clock Configuration in PTP-Supported Systems", "index.html#autotoc_md27", null ]
      ] ],
      [ "Synchronizing Display VBlanks", "index.html#autotoc_md28", null ],
      [ "Running vsync_test as a Regular User without sudo", "index.html#autotoc_md29", [
        [ "Option 1: Modifying File Permissions", "index.html#autotoc_md30", null ],
        [ "Option 2: Configuring User Group Permissions and Creating a Persistent udev Rule", "index.html#autotoc_md31", [
          [ "Step 1: Add the Resource to the Video Group", "index.html#autotoc_md32", null ],
          [ "Step 2: Modify Group Permissions", "index.html#autotoc_md33", null ],
          [ "Step 3: Add the User to the Video Group", "index.html#autotoc_md34", null ],
          [ "Step 4: Create a udev Rule for Persistent Permissions", "index.html#autotoc_md35", null ],
          [ "Step 5: Reload udev Rules", "index.html#autotoc_md36", null ],
          [ "Verifying the Changes", "index.html#autotoc_md37", null ]
        ] ]
      ] ],
      [ "Running ptp4l as a Regular User without sudo", "index.html#autotoc_md38", null ]
    ] ],
    [ "Debug Tools", "index.html#autotoc_md39", [
      [ "Debug Tool Usage", "index.html#autotoc_md40", null ]
    ] ],
    [ "Contributor Covenant Code of Conduct", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html", [
      [ "Our Pledge", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md1", null ],
      [ "Our Standards", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md2", null ],
      [ "Enforcement Responsibilities", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md3", null ],
      [ "Scope", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md4", null ],
      [ "Enforcement", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md5", null ],
      [ "Enforcement Guidelines", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md6", [
        [ "1. Correction", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md7", null ],
        [ "2. Warning", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md8", null ],
        [ "3. Temporary Ban", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md9", null ],
        [ "4. Permanent Ban", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md10", null ]
      ] ],
      [ "Attribution", "md__c_o_d_e__o_f__c_o_n_d_u_c_t.html#autotoc_md11", null ]
    ] ],
    [ "Contributing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", null ],
    [ "Release Notes for vsync v2.0.0", "md_release_notes_v2_0_0.html", [
      [ "Release Date", "md_release_notes_v2_0_0.html#autotoc_md42", null ],
      [ "Overview", "md_release_notes_v2_0_0.html#autotoc_md43", null ],
      [ "What's New", "md_release_notes_v2_0_0.html#autotoc_md44", null ],
      [ "Improvements", "md_release_notes_v2_0_0.html#autotoc_md45", null ],
      [ "Bug Fixes", "md_release_notes_v2_0_0.html#autotoc_md46", null ],
      [ "Known Issues", "md_release_notes_v2_0_0.html#autotoc_md47", null ],
      [ "Breaking Changes", "md_release_notes_v2_0_0.html#autotoc_md48", null ],
      [ "Contributors", "md_release_notes_v2_0_0.html#autotoc_md49", null ],
      [ "How to Get It", "md_release_notes_v2_0_0.html#autotoc_md50", null ],
      [ "Additional Notes", "md_release_notes_v2_0_0.html#autotoc_md51", null ]
    ] ],
    [ "Release Notes for vsync v3.0.0", "md_release_notes_v3_0_0.html", [
      [ "Release Date", "md_release_notes_v3_0_0.html#autotoc_md54", null ],
      [ "Overview", "md_release_notes_v3_0_0.html#autotoc_md55", null ],
      [ "What's New", "md_release_notes_v3_0_0.html#autotoc_md56", null ],
      [ "Bug Fixes/Enhancements", "md_release_notes_v3_0_0.html#autotoc_md57", null ],
      [ "Known Issues", "md_release_notes_v3_0_0.html#autotoc_md58", null ],
      [ "Breaking Changes", "md_release_notes_v3_0_0.html#autotoc_md59", null ],
      [ "Contributors", "md_release_notes_v3_0_0.html#autotoc_md60", null ],
      [ "How to Get It", "md_release_notes_v3_0_0.html#autotoc_md61", null ],
      [ "Additional Notes", "md_release_notes_v3_0_0.html#autotoc_md62", null ]
    ] ],
    [ "Security Policy", "md_security.html", [
      [ "Reporting a Vulnerability", "md_security.html#autotoc_md65", null ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", "globals_defs" ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"adl__p_8h.html",
"classmsg.html#a4e2cef31d72aef6c076dd9ca0c8491e7",
"globals_defs_r.html",
"vsyncalter_8h.html#afbb2401fa8ea7201da5a72c33768159e"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';