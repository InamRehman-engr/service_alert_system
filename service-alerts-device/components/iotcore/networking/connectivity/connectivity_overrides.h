#ifndef __connectivity_overrides_h__
#define __connectivity_overrides_h__

#ifdef ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
void connectivity_override_internet_availability(bool available);
#endif

#endif // __connectivity_overrides_h__