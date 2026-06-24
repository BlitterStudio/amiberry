#ifndef CAPSCORE_H
#define CAPSCORE_H

extern int api_debug_request;

int CAPSLockImage(int id, std::unique_ptr<CBaseFile> pf);
void CAPSLockTrackT0(PCAPSTRACKINFO pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag);
void CAPSLockTrackT1(PCAPSTRACKINFOT1 pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag);
void CAPSLockTrackT2(PCAPSTRACKINFOT2 pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag);
void CAPSGetVersionInfoT0(PCAPSVERSIONINFO pi);
int CAPSGetSectorInfo(PCAPSSECTORINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid);
int CAPSGetWeakInfo(PCAPSDATAINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid);
int CAPSGetRevolutionInfo(PCAPSREVOLUTIONINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid);

#endif
