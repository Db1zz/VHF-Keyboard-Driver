#pragma once

#include <pshpack1.h>

#pragma warning(disable:4214)  // suppress bit field types other than int warning
typedef struct _HID_INPUT_REPORT
{
	// Report Id for the collection
	//
	BYTE ReportId;
	union
	{
		// Input Report for Keyboard. 
		//
		struct
		{
			// Bits 15-8 are the modifier bits for key.
			//
			union
			{
				struct
				{
					BYTE LeftCtrl : 1;
					BYTE LeftShift : 1;
					BYTE LeftAlt : 1;
					BYTE LeftWin : 1;
					BYTE RightCtrl : 1;
					BYTE RightShift : 1;
					BYTE RightAlt : 1;
					BYTE RightWin : 1;
				} ModifierKeyBits;
				BYTE ModifierKeyByte;
			} ModifierKeys;
			// Bits 7-0 are the HID Usage Code
			//
			BYTE Key;
		} KeyboardInput;

		// Input Report for Consumer device.
		// Bits 15-0 are the HID Usage Code
		//
		USHORT ConsumerInput;
	} Input;
} HID_INPUT_REPORT;

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;
typedef struct _HID_DEVICE_CONTEXT
{
	WDFDEVICE				Device;
	WDFQUEUE				DefaultQueue;
	
	PHID_REPORT_DESCRIPTOR	HidReportDescriptor;
	HID_INPUT_REPORT		HidInputReport;
	USHORT					HidReportDescriptorLenghtW;
	VHFHANDLE				VhfHandle;

} HID_DEVICE_CONTEXT, *PHID_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HID_DEVICE_CONTEXT, GetHidContext)

typedef struct _QUEUE_CONTEXT
{
	WDFQUEUE				Queue;
	PHID_DEVICE_CONTEXT		DeviceContext;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext)

NTSTATUS Keyboard_EvtDriverDeviceAdd(
	_In_    WDFDRIVER		Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
);

VOID Keyboard_EvtIoDeviceControl(
	_In_ WDFQUEUE	Queue, 
	_In_ WDFREQUEST	Request,
	_In_ size_t		OutputBufferLenght,
	_In_ size_t		InputBufferLenght,
	_In_ ULONG		IoControlCode
);

NTSTATUS Keyboard_SubmitReadReport(
	_In_ PHID_DEVICE_CONTEXT DeviceContext,
	_In_ PVOID				 DataBuffer
);


NTSTATUS Keyboard_VhfHidInitialize(
	_In_ WDFDEVICE			 Device
);

NTSTATUS Keyboard_WdfIoQueueInitialize(
	_In_  WDFDEVICE			Device,
	_Out_ WDFQUEUE			*Queue
);

NTSTATUS Keyboard_EvtDeviceSelfManagedIoInit(
	_In_ WDFDEVICE			Device
);

VOID Keyboard_EvtDeviceSelfManagedIoCleanup(
	_In_ WDFDEVICE	Device
);

#define KEYBOARD_REPORT_ID		1
#define MOUSE_REPORT_ID			2
#define TOUCH_REPORT_ID			3
#define MAX_COUNT_REPORT_ID		4

#include <poppack.h>