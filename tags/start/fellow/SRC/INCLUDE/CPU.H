#ifndef CPU_H
#define CPU_H

/*===========================================================================*/
/* This section contains assembly functions exported to C                    */
/*===========================================================================*/

/* Rename assembly functions */

#define cpuPrepareException ASMRENAME(cpuPrepareException)
#define cpuPrivilegieViolation ASMRENAME(cpuPrivilegieViolation)
#define cpuUpdateSR ASMRENAME(cpuUpdateSR)
#define cpuSetUpInterrupt ASMRENAME(cpuSetUpInterrupt)
#define cpuGroup1 ASMRENAME(cpuGroup1)
#define cpuGroup2 ASMRENAME(cpuGroup2)
#define cpuFrame0 ASMRENAME(cpuFrame0)
#define cpuFrame1 ASMRENAME(cpuFrame1)
#define cpuFrame2 ASMRENAME(cpuFrame2)
#define cpuFrame3 ASMRENAME(cpuFrame3)
#define cpuFrame4 ASMRENAME(cpuFrame4)
#define cpuFrame7 ASMRENAME(cpuFrame7)
#define cpuFrame8 ASMRENAME(cpuFrame8)
#define cpuFrame9 ASMRENAME(cpuFrame9)
#define cpuFrameA ASMRENAME(cpuFrameA)
#define cpuFrameB ASMRENAME(cpuFrameB)
#define i00 ASMRENAME(i00)
#define i01_1 ASMRENAME(i01_1)
#define i01_2 ASMRENAME(i01_2)
#define i02_B_1 ASMRENAME(i02_B_1)
#define i02_W_1 ASMRENAME(i02_W_1)
#define i02_L_1 ASMRENAME(i02_L_1)
#define i02_B_2 ASMRENAME(i02_B_2)
#define i02_W_2 ASMRENAME(i02_W_2)
#define i02_L_2 ASMRENAME(i02_L_2)
#define i03_W ASMRENAME(i03_W)
#define i03_L ASMRENAME(i03_L)
#define i04_B ASMRENAME(i04_B)
#define i04_W ASMRENAME(i04_W)
#define i04_L ASMRENAME(i04_L)
#define i05_B ASMRENAME(i05_B)
#define i05_W ASMRENAME(i05_W)
#define i05_L ASMRENAME(i05_L)
#define i05_ADR ASMRENAME(i05_ADR)
#define i06_B_1 ASMRENAME(i06_B_1)
#define i06_W_1 ASMRENAME(i06_W_1)
#define i06_L_1 ASMRENAME(i06_L_1)
#define i06_B_2 ASMRENAME(i06_B_2)
#define i06_W_2 ASMRENAME(i06_W_2)
#define i06_L_2 ASMRENAME(i06_L_2)
#define i07_B_1 ASMRENAME(i07_B_1)
#define i07_W_1 ASMRENAME(i07_W_1)
#define i07_L_1 ASMRENAME(i07_L_1)
#define i07_B_2 ASMRENAME(i07_B_2)
#define i07_W_2 ASMRENAME(i07_W_2)
#define i07_L_2 ASMRENAME(i07_L_2)
#define i08_B ASMRENAME(i08_B)
#define i08_W ASMRENAME(i08_W)
#define i08_L ASMRENAME(i08_L)
#define i08_B_CCR ASMRENAME(i08_B_CCR)
#define i08_W_SR ASMRENAME(i08_W_SR)
#define i09_RI_I_B ASMRENAME(i09_RI_I_B)
#define i09_RI_I_W ASMRENAME(i09_RI_I_W)
#define i09_RI_I_L ASMRENAME(i09_RI_I_L)
#define i09_RI_R_B ASMRENAME(i09_RI_R_B)
#define i09_RI_R_W ASMRENAME(i09_RI_R_W)
#define i09_RI_R_L ASMRENAME(i09_RI_R_L)
#define i09_RI_M_W ASMRENAME(i09_RI_M_W)
#define i09_LE_I_B ASMRENAME(i09_LE_I_B)
#define i09_LE_I_W ASMRENAME(i09_LE_I_W)
#define i09_LE_I_L ASMRENAME(i09_LE_I_L)
#define i09_LE_R_B ASMRENAME(i09_LE_R_B)
#define i09_LE_R_W ASMRENAME(i09_LE_R_W)
#define i09_LE_R_L ASMRENAME(i09_LE_R_L)
#define i09_LE_M_W ASMRENAME(i09_LE_M_W)
#define i11_B ASMRENAME(i11_B)
#define i11_W ASMRENAME(i11_W)
#define i11_L ASMRENAME(i11_L)
#define i12_D_L ASMRENAME(i12_D_L)
#define i12_D_B ASMRENAME(i12_D_B)
#define i12_S_L ASMRENAME(i12_S_L)
#define i12_S_B ASMRENAME(i12_S_B)
#define i13_D_L ASMRENAME(i13_D_L)
#define i13_D_B ASMRENAME(i13_D_B)
#define i13_S_L ASMRENAME(i13_S_L)
#define i13_S_B ASMRENAME(i13_S_B)
#define i15_D_L ASMRENAME(i15_D_L)
#define i15_D_B ASMRENAME(i15_D_B)
#define i15_S_L ASMRENAME(i15_S_L)
#define i15_S_B ASMRENAME(i15_S_B)
#define i17_D_L ASMRENAME(i17_D_L)
#define i17_D_B ASMRENAME(i17_D_B)
#define i17_S_L ASMRENAME(i17_S_L)
#define i17_S_B ASMRENAME(i17_S_B)
#define i18 ASMRENAME(i18)
#define i19 ASMRENAME(i19)
#define i20_B ASMRENAME(i20_B)
#define i20_W ASMRENAME(i20_W)
#define i20_L ASMRENAME(i20_L)
#define i21_W ASMRENAME(i21_W)
#define i21_L ASMRENAME(i21_L)
#define i22_B ASMRENAME(i22_B)
#define i22_W ASMRENAME(i22_W)
#define i22_L ASMRENAME(i22_L)
#define i23_B ASMRENAME(i23_B)
#define i23_W ASMRENAME(i23_W)
#define i23_L ASMRENAME(i23_L)
#define i24 ASMRENAME(i24)
#define i25 ASMRENAME(i25)
#define i26 ASMRENAME(i26)
#define i27_B ASMRENAME(i27_B)
#define i27_W ASMRENAME(i27_W)
#define i27_L ASMRENAME(i27_L)
#define i28_B ASMRENAME(i28_B)
#define i28_W ASMRENAME(i28_W)
#define i28_L ASMRENAME(i28_L)
#define i28_B_CCR ASMRENAME(i28_B_CCR)
#define i28_W_SR ASMRENAME(i28_W_SR)
#define i29 ASMRENAME(i29)
#define i30_W ASMRENAME(i30_W)
#define i30_L ASMRENAME(i30_L)
#define i31 ASMRENAME(i31)
#define i32 ASMRENAME(i32)
#define i33 ASMRENAME(i33)
#define i34 ASMRENAME(i34)
#define i35_RI_I_B ASMRENAME(i35_RI_I_B)
#define i35_RI_I_W ASMRENAME(i35_RI_I_W)
#define i35_RI_I_L ASMRENAME(i35_RI_I_L)
#define i35_RI_R_B ASMRENAME(i35_RI_R_B)
#define i35_RI_R_W ASMRENAME(i35_RI_R_W)
#define i35_RI_R_L ASMRENAME(i35_RI_R_L)
#define i35_RI_M_W ASMRENAME(i35_RI_M_W)
#define i35_LE_I_B ASMRENAME(i35_LE_I_B)
#define i35_LE_I_W ASMRENAME(i35_LE_I_W)
#define i35_LE_I_L ASMRENAME(i35_LE_I_L)
#define i35_LE_R_B ASMRENAME(i35_LE_R_B)
#define i35_LE_R_W ASMRENAME(i35_LE_R_W)
#define i35_LE_R_L ASMRENAME(i35_LE_R_L)
#define i35_LE_M_W ASMRENAME(i35_LE_M_W)
#define i37_B ASMRENAME(i37_B)
#define i37_W ASMRENAME(i37_W)
#define i37_L ASMRENAME(i37_L)
#define i38 ASMRENAME(i38)
#define i39 ASMRENAME(i39)
#define i40 ASMRENAME(i40)
#define i41_1 ASMRENAME(i41_1)
#define i41_2 ASMRENAME(i41_2)
#define i42_W ASMRENAME(i42_W)
#define i42_L ASMRENAME(i42_L)
#define i43_W_PREM ASMRENAME(i43_W_PREM)
#define i43_L_PREM ASMRENAME(i43_L_PREM)
#define i43_W_POSTR ASMRENAME(i43_W_POSTR)
#define i43_L_POSTR ASMRENAME(i43_L_POSTR)
#define i43_W_CONM ASMRENAME(i43_W_CONM)
#define i43_L_CONM ASMRENAME(i43_L_CONM)
#define i43_W_CONR ASMRENAME(i43_W_CONR)
#define i43_L_CONR ASMRENAME(i43_L_CONR)
#define i44_W_1 ASMRENAME(i44_W_1)
#define i44_L_1 ASMRENAME(i44_L_1)
#define i44_W_2 ASMRENAME(i44_W_2)
#define i44_L_2 ASMRENAME(i44_L_2)
#define i45 ASMRENAME(i45)

#ifdef AI_REQ
#define i45ai ASMRENAME(i45ai)
#endif

#define i46 ASMRENAME(i46)
#define i47 ASMRENAME(i47)
#define i48_B ASMRENAME(i48_B)
#define i49_B ASMRENAME(i49_B)
#define i49_W ASMRENAME(i49_W)
#define i49_L ASMRENAME(i49_L)

#define i50_B ASMRENAME(i50_B)
#define i50_W ASMRENAME(i50_W)
#define i50_L ASMRENAME(i50_L)
#define i51 ASMRENAME(i51)
#define i52_B ASMRENAME(i52_B)
#define i52_W ASMRENAME(i52_W)
#define i52_L ASMRENAME(i52_L)
#define i53_B_1 ASMRENAME(i53_B_1)
#define i53_W_1 ASMRENAME(i53_W_1)
#define i53_L_1 ASMRENAME(i53_L_1)
#define i53_B_2 ASMRENAME(i53_B_2)
#define i53_W_2 ASMRENAME(i53_W_2)
#define i53_L_2 ASMRENAME(i53_L_2)
#define i54_B ASMRENAME(i54_B)
#define i54_W ASMRENAME(i54_W)
#define i54_L ASMRENAME(i54_L)
#define i54_B_CCR ASMRENAME(i54_B_CCR)
#define i54_W_SR ASMRENAME(i54_W_SR)
#define i55 ASMRENAME(i55)
#define i56 ASMRENAME(i56)
#define i57_RI_I_B ASMRENAME(i57_RI_I_B)
#define i57_RI_I_W ASMRENAME(i57_RI_I_W)
#define i57_RI_I_L ASMRENAME(i57_RI_I_L)
#define i57_RI_R_B ASMRENAME(i57_RI_R_B)
#define i57_RI_R_W ASMRENAME(i57_RI_R_W)
#define i57_RI_R_L ASMRENAME(i57_RI_R_L)
#define i57_RI_M_W ASMRENAME(i57_RI_M_W)
#define i57_LE_I_B ASMRENAME(i57_LE_I_B)
#define i57_LE_I_W ASMRENAME(i57_LE_I_W)
#define i57_LE_I_L ASMRENAME(i57_LE_I_L)
#define i57_LE_R_B ASMRENAME(i57_LE_R_B)
#define i57_LE_R_W ASMRENAME(i57_LE_R_W)
#define i57_LE_R_L ASMRENAME(i57_LE_R_L)
#define i57_LE_M_W ASMRENAME(i57_LE_M_W)
#define i59_RI_I_B ASMRENAME(i59_RI_I_B)
#define i59_RI_I_W ASMRENAME(i59_RI_I_W)
#define i59_RI_I_L ASMRENAME(i59_RI_I_L)
#define i59_RI_R_B ASMRENAME(i59_RI_R_B)
#define i59_RI_R_W ASMRENAME(i59_RI_R_W)
#define i59_RI_R_L ASMRENAME(i59_RI_R_L)
#define i59_RI_M_W ASMRENAME(i59_RI_M_W)
#define i59_LE_I_B ASMRENAME(i59_LE_I_B)
#define i59_LE_I_W ASMRENAME(i59_LE_I_W)
#define i59_LE_I_L ASMRENAME(i59_LE_I_L)
#define i59_LE_R_B ASMRENAME(i59_LE_R_B)
#define i59_LE_R_W ASMRENAME(i59_LE_R_W)
#define i59_LE_R_L ASMRENAME(i59_LE_R_L)
#define i59_LE_M_W ASMRENAME(i59_LE_M_W)
#define i61 ASMRENAME(i61)
#define i62 ASMRENAME(i62)
#define i63 ASMRENAME(i63)
#define i64_1 ASMRENAME(i64_1)
#define i64_2 ASMRENAME(i64_2)
#define i65 ASMRENAME(i65)
#define i66 ASMRENAME(i66)
#define i67_B_1 ASMRENAME(i67_B_1)
#define i67_W_1 ASMRENAME(i67_W_1)
#define i67_L_1 ASMRENAME(i67_L_1)
#define i67_B_2 ASMRENAME(i67_B_2)
#define i67_W_2 ASMRENAME(i67_W_2)
#define i67_L_2 ASMRENAME(i67_L_2)
#define i68_W ASMRENAME(i68_W)
#define i68_L ASMRENAME(i68_L)
#define i69_B ASMRENAME(i69_B)
#define i69_W ASMRENAME(i69_W)
#define i69_L ASMRENAME(i69_L)
#define i70_B ASMRENAME(i70_B)
#define i70_W ASMRENAME(i70_W)
#define i70_L ASMRENAME(i70_L)
#define i70_ADR ASMRENAME(i70_ADR)
#define cpuSubXB1 ASMRENAME(cpuSubXB1)
#define cpuSubXW1 ASMRENAME(cpuSubXW1)
#define cpuSubXL1 ASMRENAME(cpuSubXL1)
#define cpuSubXB2 ASMRENAME(cpuSubXB2)
#define cpuSubXW2 ASMRENAME(cpuSubXW2)
#define cpuSubXL2 ASMRENAME(cpuSubXL2)
#define cpuSwapW ASMRENAME(cpuSwapW)
#define cpuTasB ASMRENAME(cpuTasB)
#define cpuTrap ASMRENAME(cpuTrap)
#define cpuTrapV ASMRENAME(cpuTrapV)
#define cpuTstB ASMRENAME(cpuTstB)
#define cpuTstW ASMRENAME(cpuTstW)
#define cpuTstL ASMRENAME(cpuTstL)
#define cpuUnlk ASMRENAME(cpuUnlk)

#define cc0 ASMRENAME(cc0)
#define cc1 ASMRENAME(cc1)
#define cc2 ASMRENAME(cc2)
#define cc3 ASMRENAME(cc3)
#define cc4 ASMRENAME(cc4)
#define cc5 ASMRENAME(cc5)
#define cc6 ASMRENAME(cc6)
#define cc7 ASMRENAME(cc7)
#define cc8 ASMRENAME(cc8)
#define cc9 ASMRENAME(cc9)
#define cca ASMRENAME(cca)
#define ccb ASMRENAME(ccb)
#define ccc ASMRENAME(ccc)
#define ccd ASMRENAME(ccd)
#define cce ASMRENAME(cce)
#define ccf ASMRENAME(ccf)
#define cc1false ASMRENAME(cc1false)

#define arb00 ASMRENAME(arb00)
#define arb02 ASMRENAME(arb02)
#define arb03 ASMRENAME(arb03)
#define arb04 ASMRENAME(arb04)
#define arb05 ASMRENAME(arb05)
#define arb06 ASMRENAME(arb06)
#define arb70 ASMRENAME(arb70)
#define arb71 ASMRENAME(arb71)
#define arb72 ASMRENAME(arb72)
#define arb73 ASMRENAME(arb73)
#define arb74 ASMRENAME(arb74)

#define arw00 ASMRENAME(arw00)
#define arw01 ASMRENAME(arw01)
#define arw02 ASMRENAME(arw02)
#define arw03 ASMRENAME(arw03)
#define arw04 ASMRENAME(arw04)
#define arw05 ASMRENAME(arw05)
#define arw06 ASMRENAME(arw06)
#define arw70 ASMRENAME(arw70)
#define arw71 ASMRENAME(arw71)
#define arw72 ASMRENAME(arw72)
#define arw73 ASMRENAME(arw73)
#define arw74 ASMRENAME(arw74)

#define arl00 ASMRENAME(arl00)
#define arl01 ASMRENAME(arl01)
#define arl02 ASMRENAME(arl02)
#define arl03 ASMRENAME(arl03)
#define arl04 ASMRENAME(arl04)
#define arl05 ASMRENAME(arl05)
#define arl06 ASMRENAME(arl06)
#define arl70 ASMRENAME(arl70)
#define arl71 ASMRENAME(arl71)
#define arl72 ASMRENAME(arl72)
#define arl73 ASMRENAME(arl73)
#define arl74 ASMRENAME(arl74)

#define awb00 ASMRENAME(awb00)
#define awb02 ASMRENAME(awb02)
#define awb03 ASMRENAME(awb03)
#define awb04 ASMRENAME(awb04)
#define awb05 ASMRENAME(awb05)
#define awb06 ASMRENAME(awb06)
#define awb70 ASMRENAME(awb70)
#define awb71 ASMRENAME(awb71)

#define aww00 ASMRENAME(aww00)
#define aww01 ASMRENAME(aww01)
#define aww02 ASMRENAME(aww02)
#define aww03 ASMRENAME(aww03)
#define aww04 ASMRENAME(aww04)
#define aww05 ASMRENAME(aww05)
#define aww06 ASMRENAME(aww06)
#define aww70 ASMRENAME(aww70)
#define aww71 ASMRENAME(aww71)

#define awl00 ASMRENAME(awl00)
#define awl01 ASMRENAME(awl01)
#define awl02 ASMRENAME(awl02)
#define awl03 ASMRENAME(awl03)
#define awl04 ASMRENAME(awl04)
#define awl05 ASMRENAME(awl05)
#define awl06 ASMRENAME(awl06)
#define awl70 ASMRENAME(awl70)
#define awl71 ASMRENAME(awl71)

#define parb03 ASMRENAME(parb03)
#define parb04 ASMRENAME(parb04)
#define parb05 ASMRENAME(parb05)
#define parb06 ASMRENAME(parb06)
#define parb70 ASMRENAME(parb70)
#define parb71 ASMRENAME(parb71)
#define parb72 ASMRENAME(parb72)
#define parb73 ASMRENAME(parb73)
#define parb74 ASMRENAME(parb74)

#define parw03 ASMRENAME(parw03)
#define parw04 ASMRENAME(parw04)
#define parw05 ASMRENAME(parw05)
#define parw06 ASMRENAME(parw06)
#define parw70 ASMRENAME(parw70)
#define parw71 ASMRENAME(parw71)
#define parw72 ASMRENAME(parw72)
#define parw73 ASMRENAME(parw73)
#define parw74 ASMRENAME(parw74)

#define parl03 ASMRENAME(parl03)
#define parl04 ASMRENAME(parl04)
#define parl05 ASMRENAME(parl05)
#define parl06 ASMRENAME(parl06)
#define parl70 ASMRENAME(parl70)
#define parl71 ASMRENAME(parl71)
#define parl72 ASMRENAME(parl72)
#define parl73 ASMRENAME(parl73)
#define parl74 ASMRENAME(parl74)

#define eac02 ASMRENAME(eac02)
#define eac05 ASMRENAME(eac05)
#define eac06 ASMRENAME(eac06)
#define eac70 ASMRENAME(eac70)
#define eac71 ASMRENAME(eac71)
#define eac72 ASMRENAME(eac72)
#define eac73 ASMRENAME(eac73)

#define i301_M ASMRENAME(i301_M)
#define i301_R ASMRENAME(i301_R)
#define i302_M ASMRENAME(i302_M)
#define i302_R ASMRENAME(i302_R)
#define i303_M ASMRENAME(i303_M)
#define i303_R ASMRENAME(i303_R)
#define i304_M ASMRENAME(i304_M)
#define i304_R ASMRENAME(i304_R)
#define i305_M ASMRENAME(i305_M)
#define i305_R ASMRENAME(i305_R)
#define i306_M ASMRENAME(i306_M)
#define i306_R ASMRENAME(i306_R)
#define i307_M ASMRENAME(i307_M)
#define i307_R ASMRENAME(i307_R)
#define i308_M ASMRENAME(i308_M)
#define i308_R ASMRENAME(i308_R)
#define i309_B ASMRENAME(i309_B)
#define i309_W ASMRENAME(i309_W)
#define i309_L ASMRENAME(i309_L)
#define i310_W ASMRENAME(i310_W)
#define i310_L ASMRENAME(i310_L)
#define i311 ASMRENAME(i311)
#define i312_B ASMRENAME(i312_B)
#define i312_W ASMRENAME(i312_W)
#define i312_L ASMRENAME(i312_L)
#define i313_W ASMRENAME(i313_W)
#define i313_L ASMRENAME(i313_L)
#define i314 ASMRENAME(i314)
#define i315 ASMRENAME(i315)
#define i316 ASMRENAME(i316)
#define i317 ASMRENAME(i317)
#define i318 ASMRENAME(i318)
#define i319 ASMRENAME(i319)
#define i320 ASMRENAME(i320)
#define i321 ASMRENAME(i321)
#define i322 ASMRENAME(i322)
#define i323 ASMRENAME(i323)
#define i324_TO ASMRENAME(i324_TO)
#define i324_FROM ASMRENAME(i324_FROM)
#define i325_B ASMRENAME(i325_B)
#define i325_W ASMRENAME(i325_W)
#define i325_L ASMRENAME(i325_L)
#define i326 ASMRENAME(i326)
#define i327 ASMRENAME(i327)
#define i328 ASMRENAME(i328)
#define MMUIllegal030 ASMRENAME(MMUIllegal030)
#define MMU030 ASMRENAME(MMU030)
#define PFLUSHEC040 ASMRENAME(PFLUSHEC040)
#define PTESTEC040 ASMRENAME(PTESTEC040)
#define i329 ASMRENAME(i329)
#define i330 ASMRENAME(i330)
#define i331 ASMRENAME(i331)
#define i332 ASMRENAME(i322)
#define i333 ASMRENAME(i333)
#define i334_1 ASMRENAME(i334_1)
#define i334_2_00 ASMRENAME(i334_2_00)
#define i334_2_01 ASMRENAME(i334_2_01)
#define i334_2_10 ASMRENAME(i334_2_10)
#define i334_2_11 ASMRENAME(i334_2_11)
#define CINV040 ASMRENAME(CINV040)
#define CPUSH040 ASMRENAME(CPUSH040)
#define arb063 ASMRENAME(arb063)
#define arw063 ASMRENAME(arw063)
#define arl063 ASMRENAME(arl063)
#define awb063 ASMRENAME(awb063)
#define aww063 ASMRENAME(aww063)
#define awl063 ASMRENAME(awl063)
#define parb063 ASMRENAME(parb063)
#define parw063 ASMRENAME(parw063)
#define parl063 ASMRENAME(parl063)
#define eac063 ASMRENAME(eac063)
#define arb0733 ASMRENAME(arb0733)
#define arw0733 ASMRENAME(arw0733)
#define arl0733 ASMRENAME(arl0733)
#define eac0733 ASMRENAME(eac0733)
#define parb0733 ASMRENAME(parb0733)
#define parw0733 ASMRENAME(parw0733)
#define parl0733 ASMRENAME(parl0733)
#define eac063is0 ASMRENAME(eac063is0)
#define eac063is1 ASMRENAME(eac063is1)
#define eac063is2 ASMRENAME(eac063is2)
#define eac063is3 ASMRENAME(eac063is3)
#define eac063is4 ASMRENAME(eac063is4)
#define eac063is5 ASMRENAME(eac063is5)
#define eac063is6 ASMRENAME(eac063is6)
#define eac063is7 ASMRENAME(eac063is7)


/* Functions actually called from C */

/* Functions that have their address taken in C */

extern void cpuPrepareException(void);
extern void cpuPrivilegieViolation(void);
extern void cpuUpdateSR(void);
extern void cpuSetUpInterrupt(void);
extern void cpuGroup1(ULO vbr);
extern void cpuGroup2(ULO vbr);
extern void cpuFrame0(ULO vbr);
extern void cpuFrame1(ULO vbr);
extern void cpuFrame2(ULO vbr);
extern void cpuFrame3(ULO vbr);
extern void cpuFrame4(ULO vbr);
extern void cpuFrame7(ULO vbr);
extern void cpuFrame8(ULO vbr);
extern void cpuFrame9(ULO vbr);
extern void cpuFrameA(ULO vbr);
extern void cpuFrameB(ULO vbr);
extern void i00(ULO);
extern void i01_1(ULO);
extern void i01_2(ULO);
extern void i02_B_1(ULO);
extern void i02_W_1(ULO);
extern void i02_L_1(ULO);
extern void i02_B_2(ULO);
extern void i02_W_2(ULO);
extern void i02_L_2(ULO);
extern void i03_W(ULO);
extern void i03_L(ULO);
extern void i04_B(ULO);
extern void i04_W(ULO);
extern void i04_L(ULO);
extern void i05_B(ULO);
extern void i05_W(ULO);
extern void i05_L(ULO);
extern void i05_ADR(ULO);
extern void i06_B_1(ULO);
extern void i06_W_1(ULO);
extern void i06_L_1(ULO);
extern void i06_B_2(ULO);
extern void i06_W_2(ULO);
extern void i06_L_2(ULO);
extern void i07_B_1(ULO);
extern void i07_W_1(ULO);
extern void i07_L_1(ULO);
extern void i07_B_2(ULO);
extern void i07_W_2(ULO);
extern void i07_L_2(ULO);
extern void i08_B(ULO);
extern void i08_W(ULO);
extern void i08_L(ULO);
extern void i08_B_CCR(ULO);
extern void i08_W_SR(ULO);
extern void i09_RI_I_B(ULO);
extern void i09_RI_I_W(ULO);
extern void i09_RI_I_L(ULO);
extern void i09_RI_R_B(ULO);
extern void i09_RI_R_W(ULO);
extern void i09_RI_R_L(ULO);
extern void i09_RI_M_W(ULO);
extern void i09_LE_I_B(ULO);
extern void i09_LE_I_W(ULO);
extern void i09_LE_I_L(ULO);
extern void i09_LE_R_B(ULO);
extern void i09_LE_R_W(ULO);
extern void i09_LE_R_L(ULO);
extern void i09_LE_M_W(ULO);
extern void i11_B(ULO);
extern void i11_W(ULO);
extern void i11_L(ULO);
extern void i12_D_L(ULO);
extern void i12_D_B(ULO);
extern void i12_S_L(ULO);
extern void i12_S_B(ULO);
extern void i13_D_L(ULO);
extern void i13_D_B(ULO);
extern void i13_S_L(ULO);
extern void i13_S_B(ULO);
extern void i15_D_L(ULO);
extern void i15_D_B(ULO);
extern void i15_S_L(ULO);
extern void i15_S_B(ULO);
extern void i17_D_L(ULO);
extern void i17_D_B(ULO);
extern void i17_S_L(ULO);
extern void i17_S_B(ULO);
extern void i18(ULO);
extern void i19(ULO);
extern void i20_B(ULO);
extern void i20_W(ULO);
extern void i20_L(ULO);
extern void i21_W(ULO);
extern void i21_L(ULO);
extern void i22_B(ULO);
extern void i22_W(ULO);
extern void i22_L(ULO);
extern void i23_B(ULO);
extern void i23_W(ULO);
extern void i23_L(ULO);
extern void i24(ULO);
extern void i25(ULO);
extern void i26(ULO);
extern void i27_B(ULO);
extern void i27_W(ULO);
extern void i27_L(ULO);
extern void i28_B(ULO);
extern void i28_W(ULO);
extern void i28_L(ULO);
extern void i28_B_CCR(ULO);
extern void i28_W_SR(ULO);
extern void i29(ULO);
extern void i30_W(ULO);
extern void i30_L(ULO);
extern void i31(ULO);
extern void i32(ULO);
extern void i33(ULO);
extern void i34(ULO);
extern void i35_RI_I_B(ULO);
extern void i35_LE_I_B(ULO);
extern void i35_RI_I_W(ULO);
extern void i35_LE_I_W(ULO);
extern void i35_RI_I_L(ULO);
extern void i35_LE_I_L(ULO);
extern void i35_RI_R_B(ULO);
extern void i35_LE_R_B(ULO);
extern void i35_RI_R_W(ULO);
extern void i35_LE_R_W(ULO);
extern void i35_RI_R_L(ULO);
extern void i35_LE_R_L(ULO);
extern void i35_RI_M_W(ULO);
extern void i35_LE_M_W(ULO);
extern void i37_B(ULO);
extern void i37_W(ULO);
extern void i37_L(ULO);
extern void i38(ULO);
extern void i39(ULO);
extern void i40(ULO);
extern void i41_1(ULO);
extern void i41_2(ULO);
extern void i42_W(ULO);
extern void i42_L(ULO);
extern void i43_W_PREM(ULO);
extern void i43_L_PREM(ULO);
extern void i43_W_POSTR(ULO);
extern void i43_L_POSTR(ULO);
extern void i43_W_CONM(ULO);
extern void i43_L_CONM(ULO);
extern void i43_W_CONR(ULO);
extern void i43_L_CONR(ULO);
extern void i44_W_1(ULO);
extern void i44_L_1(ULO);
extern void i44_W_2(ULO);
extern void i44_L_2(ULO);
extern void i45(ULO);

#ifdef AI_REQ
extern void i45ai(ULO);
#endif

extern void i46(ULO);
extern void i47(ULO);
extern void i48_B(ULO);
extern void i49_B(ULO);
extern void i49_W(ULO);
extern void i49_L(ULO);

extern void i50_B(ULO);
extern void i50_W(ULO);
extern void i50_L(ULO);
extern void i51(ULO);
extern void i52_B(ULO);
extern void i52_W(ULO);
extern void i52_L(ULO);
extern void i53_B_1(ULO);
extern void i53_W_1(ULO);
extern void i53_L_1(ULO);
extern void i53_B_2(ULO);
extern void i53_W_2(ULO);
extern void i53_L_2(ULO);
extern void i54_B(ULO);
extern void i54_W(ULO);
extern void i54_L(ULO);
extern void i54_B_CCR(ULO);
extern void i54_W_SR(ULO);
extern void i55(ULO);
extern void i56(ULO);
extern void i57_RI_I_B(ULO);
extern void i57_LE_I_B(ULO);
extern void i57_RI_I_W(ULO);
extern void i57_LE_I_W(ULO);
extern void i57_RI_I_L(ULO);
extern void i57_LE_I_L(ULO);
extern void i57_RI_R_B(ULO);
extern void i57_LE_R_B(ULO);
extern void i57_RI_R_W(ULO);
extern void i57_LE_R_W(ULO);
extern void i57_RI_R_L(ULO);
extern void i57_LE_R_L(ULO);
extern void i57_RI_M_W(ULO);
extern void i57_LE_M_W(ULO);
extern void i59_RI_I_B(ULO);
extern void i59_LE_I_B(ULO);
extern void i59_RI_I_W(ULO);
extern void i59_LE_I_W(ULO);
extern void i59_RI_I_L(ULO);
extern void i59_LE_I_L(ULO);
extern void i59_RI_R_B(ULO);
extern void i59_LE_R_B(ULO);
extern void i59_RI_R_W(ULO);
extern void i59_LE_R_W(ULO);
extern void i59_RI_R_L(ULO);
extern void i59_LE_R_L(ULO);
extern void i59_RI_M_W(ULO);
extern void i59_LE_M_W(ULO);
extern void i61(ULO);
extern void i62(ULO);
extern void i63(ULO);
extern void SBCD_DREG(ULO);
extern void SBCD_PDEC(ULO);
extern void i64_1(ULO);
extern void i64_2(ULO);
extern void i65(ULO);
extern void i66(ULO);
extern void i67_B_1(ULO);
extern void i67_W_1(ULO);
extern void i67_L_1(ULO);
extern void i67_B_2(ULO);
extern void i67_W_2(ULO);
extern void i67_L_2(ULO);
extern void i68_W(ULO);
extern void i68_L(ULO);
extern void i69_B(ULO);
extern void i69_W(ULO);
extern void i69_L(ULO);
extern void i70_B(ULO);
extern void i70_W(ULO);
extern void i70_L(ULO);
extern void i70_ADR(ULO);
extern void cpuSubXB1(ULO);
extern void cpuSubXW1(ULO);
extern void cpuSubXL1(ULO);
extern void cpuSubXB2(ULO);
extern void cpuSubXW2(ULO);
extern void cpuSubXL2(ULO);
extern void cpuSwapW(ULO);
extern void cpuTasB(ULO);
extern void cpuTrap(ULO);
extern void cpuTrapV(ULO);
extern void cpuTstB(ULO);
extern void cpuTstW(ULO);
extern void cpuTstL(ULO);
extern void cpuUnlk(ULO);

extern void cc0(void);
extern void cc1(void);
extern void cc2(void);
extern void cc3(void);
extern void cc4(void);
extern void cc5(void);
extern void cc6(void);
extern void cc7(void);
extern void cc8(void);
extern void cc9(void);
extern void cca(void);
extern void ccb(void);
extern void ccc(void);
extern void ccd(void);
extern void cce(void);
extern void ccf(void);
extern void cc1false(void);

extern ULO arb00(ULO);
extern ULO arb02(ULO);
extern ULO arb03(ULO);
extern ULO arb04(ULO);
extern ULO arb05(ULO);
extern ULO arb06(ULO);
extern ULO arb70(ULO);
extern ULO arb71(ULO);
extern ULO arb72(ULO);
extern ULO arb73(ULO);
extern ULO arb74(ULO);

extern ULO arw00(ULO);
extern ULO arw01(ULO);
extern ULO arw02(ULO);
extern ULO arw03(ULO);
extern ULO arw04(ULO);
extern ULO arw05(ULO);
extern ULO arw06(ULO);
extern ULO arw70(ULO);
extern ULO arw71(ULO);
extern ULO arw72(ULO);
extern ULO arw73(ULO);
extern ULO arw74(ULO);

extern ULO arl00(ULO);
extern ULO arl01(ULO);
extern ULO arl02(ULO);
extern ULO arl03(ULO);
extern ULO arl04(ULO);
extern ULO arl05(ULO);
extern ULO arl06(ULO);
extern ULO arl70(ULO);
extern ULO arl71(ULO);
extern ULO arl72(ULO);
extern ULO arl73(ULO);
extern ULO arl74(ULO);

extern void awb00(ULO,ULO);
extern void awb02(ULO,ULO);
extern void awb03(ULO,ULO);
extern void awb04(ULO,ULO);
extern void awb05(ULO,ULO);
extern void awb06(ULO,ULO);
extern void awb70(ULO,ULO);
extern void awb71(ULO,ULO);

extern void aww00(ULO,ULO);
extern void aww01(ULO,ULO);
extern void aww02(ULO,ULO);
extern void aww03(ULO,ULO);
extern void aww04(ULO,ULO);
extern void aww05(ULO,ULO);
extern void aww06(ULO,ULO);
extern void aww70(ULO,ULO);
extern void aww71(ULO,ULO);

extern void awl00(ULO,ULO);
extern void awl01(ULO,ULO);
extern void awl02(ULO,ULO);
extern void awl03(ULO,ULO);
extern void awl04(ULO,ULO);
extern void awl05(ULO,ULO);
extern void awl06(ULO,ULO);
extern void awl70(ULO,ULO);
extern void awl71(ULO,ULO);

extern ULO parb03(ULO);
extern ULO parb04(ULO);
extern ULO parb05(ULO);
extern ULO parb06(ULO);
extern ULO parb70(ULO);
extern ULO parb71(ULO);
extern ULO parb72(ULO);
extern ULO parb73(ULO);
extern ULO parb74(ULO);

extern ULO parw03(ULO);
extern ULO parw04(ULO);
extern ULO parw05(ULO);
extern ULO parw06(ULO);
extern ULO parw70(ULO);
extern ULO parw71(ULO);
extern ULO parw72(ULO);
extern ULO parw73(ULO);
extern ULO parw74(ULO);

extern ULO parl03(ULO);
extern ULO parl04(ULO);
extern ULO parl05(ULO);
extern ULO parl06(ULO);
extern ULO parl70(ULO);
extern ULO parl71(ULO);
extern ULO parl72(ULO);
extern ULO parl73(ULO);
extern ULO parl74(ULO);

extern ULO eac02(ULO);
extern ULO eac05(ULO);
extern ULO eac06(ULO);
extern ULO eac70(ULO);
extern ULO eac71(ULO);
extern ULO eac72(ULO);
extern ULO eac73(ULO);

extern void i301_M(ULO);
extern void i301_R(ULO);
extern void i302_M(ULO);
extern void i302_R(ULO);
extern void i303_M(ULO);
extern void i303_R(ULO);
extern void i304_M(ULO);
extern void i304_R(ULO);
extern void i305_M(ULO);
extern void i305_R(ULO);
extern void i306_M(ULO);
extern void i306_R(ULO);
extern void i307_M(ULO);
extern void i307_R(ULO);
extern void i308_M(ULO);
extern void i308_R(ULO);
extern void i309_B(ULO);
extern void i309_W(ULO);
extern void i309_L(ULO);
extern void i310_W(ULO);
extern void i310_L(ULO);
extern void i311(ULO);
extern void i312_B(ULO);
extern void i312_W(ULO);
extern void i312_L(ULO);
extern void i313_W(ULO);
extern void i313_L(ULO);
extern void i314(ULO);
extern void i315(ULO);
extern void i316(ULO);
extern void i317(ULO);
extern void i318(ULO);
extern void i319(ULO);
extern void i320(ULO);
extern void i321(ULO);
extern void i322(ULO);
extern void i323(ULO);
extern void i324_TO(ULO);
extern void i324_FROM(ULO);
extern void i325_B(ULO);
extern void i325_W(ULO);
extern void i325_L(ULO);
extern void i326(ULO);
extern void i327(ULO);
extern void i328(ULO);
extern void MMUIllegal030(ULO);
extern void MMU030(ULO);
extern void PFLUSHEC040(ULO);
extern void PTESTEC040(ULO);
extern void i329(ULO);
extern void i330(ULO);
extern void i331(ULO);
extern void i332(ULO);
extern void i333(ULO);
extern void i334_1(ULO);
extern void i334_2_00(ULO);
extern void i334_2_01(ULO);
extern void i334_2_10(ULO);
extern void i334_2_11(ULO);
extern void CINV040(ULO);
extern void CPUSH040(ULO);
extern ULO arb063(ULO);
extern ULO arw063(ULO);
extern ULO arl063(ULO);
extern void awb063(ULO,ULO);
extern void aww063(ULO,ULO);
extern void awl063(ULO,ULO);
extern ULO parb063(ULO);
extern ULO parw063(ULO);
extern ULO parl063(ULO);
extern ULO eac063(ULO);
extern ULO arb0733(ULO);
extern ULO arw0733(ULO);
extern ULO arl0733(ULO);
extern ULO eac0733(ULO);
extern ULO parb0733(ULO);
extern ULO parw0733(ULO);
extern ULO parl0733(ULO);

extern void eac063is0(void);
extern void eac063is1(void);
extern void eac063is2(void);
extern void eac063is3(void);
extern void eac063is4(void);
extern void eac063is5(void);
extern void eac063is6(void);
extern void eac063is7(void);

/* End of assembly function export                                           */
/*===========================================================================*/

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

typedef enum {
  M68000  = 0,
  M68010  = 1,
  M68020  = 2,
  M68030  = 3,
  M68EC30 = 4,
  M68040  = 5,
  M68EC40 = 6,
  M68060  = 7,
  M68EC60 = 8,
  M68EC20 = 9
} cpu_types;

extern cpu_types cpu_type;

/* Major is x in 680X0, minor is: 0 - full, 1 - EC */

extern ULO cpu_major, cpu_minor;

extern ULO cpu_speed;

extern BOOLE cpuSetType(cpu_types type);
extern cpu_types cpuGetType(void);
extern void cpuSetSpeed(ULO speed);
extern ULO cpuGetSpeed(void);


/* Data used when initializing the CPU-emu */

#define SREG  0x00000007
#define SMOD  0x00000038
#define DMOD  0x000001c0
#define DREG  0x00000e00
#define SIZE  0x000000c0
#define SIZE3 0x00003000
#define FBTYPE 0x00000f00

#define BIT0  0x00000001
#define BIT1  0x00000002
#define BIT2  0x00000004
#define BIT3  0x00000008
#define BIT4  0x00000010
#define BIT5  0x00000020
#define BIT6  0x00000040
#define BIT7  0x00000080
#define BIT8  0x00000100
#define BIT9  0x00000200
#define BIT10 0x00000400
#define BIT11 0x00000800
#define BIT12 0x00001000
#define BIT13 0x00002000
#define BIT14 0x00004000
#define BIT15 0x00008000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000


typedef void (*cpuStackFrameFunc)(ULO);


extern void m68000start(void);   /* Code alignment label */

extern ULO cpu_next_instruction(void);

#ifdef PREFETCH
extern ULO prefetch_word;
extern ULO prefetch_pos;
extern ULO prefetch_size;
#endif


extern ULO cpu_next, irq_next;

extern ULO exceptionstack;
extern ULO exceptionbackdooraddress;
extern void next_instruction(void);
extern ULO pc;
extern ULO thiscycle;
extern ULO t[65536][8];
extern ULO a[8],d[8];
extern ULO sr,usp,ssp;
extern ULO cpuspeed;
extern ULO interruptflag;
extern ULO statustab[4096];
extern ULO ccrlo_new, ccrhi_new;

#ifdef PC_PTR
extern UBY *pcbaseadr;
#endif

typedef ULO (*disFunc)(ULO prc, ULO opc, STR *st);  
extern disFunc cpu_dis_tab[65536];

#ifdef PROF68
void dump_profile(void);
void init_profile(void);
#endif

extern void cpuSetPC(ULO address);
extern ULO cpuGetPC(ULO address);

extern const UBY allmodes[16];
extern const UBY data[16];
extern const UBY memory[16];
extern const UBY control[16];
extern const UBY alterable[16];
extern UBY tarb[12];
extern UBY tarw[12];
extern UBY tarl[12];
extern eareadfunc arb[12],arw[12],arl[12],parb[12],parw[12],parl[12];
extern eawritefunc awb[12],aww[12],awl[12];
extern eacalcfunc eac[12];
extern eareadfunc arb68000[12];
extern eareadfunc arw68000[12];
extern eareadfunc arl68000[12];
extern eareadfunc parb68000[12];
extern eareadfunc parw68000[12];
extern eareadfunc parl68000[12];
extern eawritefunc awb68000[12];
extern eawritefunc aww68000[12];
extern eawritefunc awl68000[12];
extern eacalcfunc eac68000[12];
extern const UBY tarb68000[12];
extern const UBY tarw68000[12];
extern const UBY tarl68000[12];
extern UBY ccctt[16];
extern UBY ccdtt[16];
extern UBY ccett[16];
extern UBY ccftt[16];
extern UBY hex2bcd[256];
extern UBY bcd2hex[256];
extern void i44ini(void);
ULO disam(ULO reg, ULO pcp, char *st, ULO size, ULO mode, ULO *pos);
ULO get_smod(ULO opc);
ULO get_sreg(ULO opc);
ULO get_dmod(ULO opc);
ULO get_dreg(ULO opc);
ULO get_size(ULO opc);
ULO get_size2(ULO opc);
ULO get_size3(ULO opc);
ULO get_size4(ULO opc);
char sizec(ULO size);
ULO get_branchtype(ULO opc);
ULO modreg(ULO modes, ULO regs);
ULO greg(ULO mode, ULO reg);


extern ULO d[8];
extern ULO a[8];
extern char *btab[16];
extern ULO t[65536][8];

extern ULO vbr, msp;
extern ULO cacr;

extern void cpuReset010(void);
extern void cpuReset020(void);
extern void cpuReset030(void);
extern void cpuReset040(void);
extern void cpuReset060(void);
extern void cpuInit000(void);
extern void cpuInit010(void);
extern void cpuInit020(void);
extern void cpuInit030(void);
extern void cpuInit040(void);
extern void cpuInit060(void);
extern void cpuAdrModeTablesInit020(void);


/* External prototypes for the instructions
   They return the number of cycles used on a MC68000
   The disassembly routines return the instruction length in bytes */

extern void next_instruction(void);
extern ULO get_pc(ULO);

ULO i37dis(ULO prc, ULO opc, char *st);
ULO i00dis(ULO prc, ULO opc, STR *st);

extern void cpuHardReset(void);
extern void cpuEmulationStart(void);
extern void cpuEmulationStop(void);
extern void cpuStartup(void);
extern void cpuShutdown(void);

#endif
