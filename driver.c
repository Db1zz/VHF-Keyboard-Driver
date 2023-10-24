#include <ntddk.h>
#include <hidport.h>
#include <wdf.h>
#include <vhf.h>
#include <stdbool.h>
#include "Common.h"
#include "driver.h"

#define REPORT_ID_KEYBOARD 0x01
#define REPORT_ID_CONSUMER 0x02

HID_REPORT_DESCRIPTOR
g_VirtualHidKeyboard_HidReportDescriptor[] =
{
	/*
	Value Item
	*/
	0x05, 0x01,     /* Usage Page(Generic Desktop), */
	0x09, 0x06,     /* Usage(Keyboard),*/
	0xA1, 0x01,     /* Collection(HID_FLAGS_COLLECTION_Application),*/
	0x85, REPORT_ID_KEYBOARD,  /* Report Id,*/
	0x05, 0x07,     /* Usage Page(Key Codes),*/
	0x19, 0xE0,     /* Usage Minimum(Left Ctrl),*/
	0x29, 0xE7,     /* Usage Maximum(Right Win),*/
	0x15, 0x00,     /* Logical Minimum(0),*/
	0x25, 0x01,     /* Logical Maximum(1),*/
	0x75, 0x01,     /* Report Size(1),*/
	0x95, 0x08,     /* Report Count(8),*/
	0x81, 0x02,     /* Input(Data, Variable, Absolute),*/
	0x95, 0x01,     /* Report Count(1),*/
	0x75, 0x08,     /* Report Size(8),*/
	0x25, 0x65,     /* Logical Maximum(101),*/
	0x19, 0x00,     /* Usage Minimum(0),*/
	0x29, 0x65,     /* Usage Maximum(101),*/
	0x81, 0x00,     /* Input(Data, Array),*/
	0xC0,           /* End Collection */

	0x05, 0x0C,          /* USAGE_PAGE (Consumer devices), */
	0x09, 0x01,          /* USAGE (Consumer Control) */
	0xa1, 0x01,          /* COLLECTION (HID_FLAGS_COLLECTION_Application) */
	0x85, REPORT_ID_CONSUMER,  /* Report Id, */
	0x1A, 0x00, 0x00,    /* Usage Minimum(0x0),*/
	0x2A, 0xFF, 0x03,    /* Usage Maximum(0x3FF),*/
	0x16, 0x00, 0x00,    /* Logical Minimum(0),*/
	0x26, 0xFF, 0x03,    /* Logical Maximum(1023),*/
	0x75, 0x10,          /* Report Size(16),*/
	0x95, 0x01,          /* Report Count(1),*/
	0x81, 0x00,          /* Input(Data, Array),*/
	0xc0,                /* END_COLLECTION */
};


#define MAX_HID_REPORT_SIZE sizeof(HIDINJECTOR_INPUT_REPORT)

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject, 
	_In_ PUNICODE_STRING RegistryPath
	)
{
	KdPrint(("The VHF Keyboard driver is Loading...\n"));

	WDF_DRIVER_CONFIG config;
	WDF_DRIVER_CONFIG_INIT(&config, Keyboard_EvtDriverDeviceAdd);

	NTSTATUS status = WdfDriverCreate(
						DriverObject, 
						RegistryPath,
						WDF_NO_OBJECT_ATTRIBUTES,
						&config, 
						WDF_NO_HANDLE);

	if (!NT_SUCCESS(status))
		KdPrint(("WdfDriverCreate Error: (0x%08X)\n", status));

	return status;

}

NTSTATUS Keyboard_EvtDriverDeviceAdd(
	 _In_   WDFDRIVER		Driver,
	_Inout_ PWDFDEVICE_INIT	DeviceInit
	)
{	
	UNREFERENCED_PARAMETER			(Driver);
	WDFDEVICE						device;
	NTSTATUS						status;
	WDFQUEUE						queue;
	WDF_OBJECT_ATTRIBUTES			deviceAttributes;
	WDF_PNPPOWER_EVENT_CALLBACKS	wdfPnpPowerCallbacks;
	PHID_DEVICE_CONTEXT				deviceContext;
	PQUEUE_CONTEXT					queueContext;

	PAGED_CODE();

	//WdfFdoInitSetFilter(DeviceInit);
	
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&wdfPnpPowerCallbacks);
	wdfPnpPowerCallbacks.EvtDeviceSelfManagedIoInit = Keyboard_EvtDeviceSelfManagedIoInit;
	wdfPnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = Keyboard_EvtDeviceSelfManagedIoCleanup;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &wdfPnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, HID_DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, 
							 &deviceAttributes, 
							 &device);
	if (!NT_SUCCESS(status)){
		KdPrint(("WdfDeviceCreate Error: (0x%08X)\n", status));
		goto Error;
	}

	status = Keyboard_WdfIoQueueInitialize(device, &queue);
	if (!NT_SUCCESS(status)) {
		KdPrint(("KEYBOARD_WdfIoQueueInitialize failed with error: (0x%08X)\n", status));
		goto Error;
	}

	queueContext =	GetQueueContext(queue);
	deviceContext = GetHidContext(device);
	queueContext->DeviceContext = deviceContext;

	deviceContext->HidReportDescriptor = g_VirtualHidKeyboard_HidReportDescriptor;
	deviceContext->HidReportDescriptorLenghtW =	(USHORT)sizeof(g_VirtualHidKeyboard_HidReportDescriptor);

	RtlZeroMemory(&deviceContext->HidInputReport,
		    sizeof(deviceContext->HidInputReport));

	status = Keyboard_VhfHidInitialize(device);
	if (!NT_SUCCESS(status)){
		KdPrint(("KEYBOARD_VhfHidInitialize failed with error: (0x%08X)\n", status));
		goto Error;
	}

	if (deviceContext->VhfHandle == NULL){
		KdPrint(("VHFHANDLE is NULL"));
		goto Error;
	}

	if(NT_SUCCESS(status))
		KdPrint(("Keyboard device has been successfully installed!\n"));

Error:
	return status;
}

NTSTATUS Keyboard_WdfIoQueueInitialize(
	_In_  WDFDEVICE			Device, 
	_Out_ WDFQUEUE			*Queue
	)
{
	NTSTATUS				status;
	WDF_IO_QUEUE_CONFIG		queueConfig;
	WDF_OBJECT_ATTRIBUTES	queueAttributes;

	PAGED_CODE();

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
		&queueAttributes,
		QUEUE_CONTEXT);

	queueConfig.EvtIoDeviceControl = Keyboard_EvtIoDeviceControl;

	do
	{
		status = WdfDeviceCreateDeviceInterface(Device, &GUID_Keyboard, NULL);
		if (!NT_SUCCESS(status)) {
			KdPrint(("WdfDeviceCreateDeviceInterface Error: (0x%08X)\n", status));
			break;
		}

		status = WdfIoQueueCreate(Device, &queueConfig, &queueAttributes, Queue);
		if (!NT_SUCCESS(status)) {
			KdPrint(("WdfIoQueueCreate Error: (0x%08X)\n", status));
			break;
		}

	} while (false);



	return status;
}

NTSTATUS Keyboard_VhfHidInitialize(
	_In_ WDFDEVICE				Device
	)
{
	NTSTATUS					status;
	VHF_CONFIG					vhfConfig;
	PHID_DEVICE_CONTEXT			deviceContext;

	PAGED_CODE();

	deviceContext = GetHidContext(Device);

	do {
		
		VHF_CONFIG_INIT(&vhfConfig,
						WdfDeviceWdmGetDeviceObject(Device),
						(USHORT)(deviceContext->HidReportDescriptorLenghtW),
						(UCHAR*)deviceContext->HidReportDescriptor);

		vhfConfig.VendorID = 235;
		vhfConfig.ProductID = 1;
		vhfConfig.VersionNumber = 1;

		status = VhfCreate(&vhfConfig, &deviceContext->VhfHandle);
		if (!NT_SUCCESS(status)) {
			KdPrint(("VhfCreate Error: (0x%08X)\n", status));
			break;
		}

		status = VhfStart(deviceContext->VhfHandle);
		if (!NT_SUCCESS(status)) {
			KdPrint(("VhfStart Error: (0x%08X)\n", status));
			break;
		}

	} while (false);

	return status;
}

NTSTATUS Keyboard_EvtDeviceSelfManagedIoInit(
	_In_ WDFDEVICE			Device
	)
{
	NTSTATUS			status;
	PHID_DEVICE_CONTEXT deviceContext;

	PAGED_CODE();

	deviceContext = GetHidContext(Device);

	status = VhfStart(deviceContext->VhfHandle);
	if (!NT_SUCCESS(status))
		KdPrint(("VhfStart Failed with status: (0x%08X)\n", status));

	return status;
}

VOID Keyboard_EvtDeviceSelfManagedIoCleanup(
	_In_ WDFDEVICE	Device
	)
{
	PHID_DEVICE_CONTEXT deviceContext;
	
	PAGED_CODE();

	deviceContext = GetHidContext(Device);

	VhfDelete(deviceContext->VhfHandle, TRUE);

	return;
}

VOID Keyboard_EvtIoDeviceControl(
	_In_ WDFQUEUE	Queue,
	_In_ WDFREQUEST	Request,
	_In_ size_t		OutputBufferLenght,
	_In_ size_t		InputBufferLenght,
	_In_ ULONG		IoControlCode
	)
{
	UNREFERENCED_PARAMETER(OutputBufferLenght);
	UNREFERENCED_PARAMETER(IoControlCode);

	NTSTATUS			status;
	PQUEUE_CONTEXT		queueContext;
	PHID_DEVICE_CONTEXT	deviceContext;
	WDFMEMORY			memory;
	PVOID				pvoid;
	size_t				lenght = {0};

	status = WdfRequestRetrieveInputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfRequestRetrieveInputMemory failed: (0x%08X)\n", status));
		WdfRequestComplete(Request, STATUS_INVALID_BUFFER_SIZE);
		return;
	}

	pvoid = WdfMemoryGetBuffer(memory, &InputBufferLenght);
	if (pvoid == NULL) {
		KdPrint(("WdfMemoryGetBuffer failed!"));
		WdfRequestComplete(Request, STATUS_INVALID_BUFFER_SIZE);
		return;
	}

	queueContext = GetQueueContext(Queue);
	deviceContext = queueContext->DeviceContext;

	status = Keyboard_SubmitReadReport(deviceContext, pvoid);
	if (NT_SUCCESS(status))
		KdPrint(("Report has been successfully sent!"));

	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, lenght);
	KdPrint(("Request Completed!"));
}

NTSTATUS Keyboard_SubmitReadReport(
	_In_ PHID_DEVICE_CONTEXT DeviceContext,
	_In_ PVOID				 DataBuffer
	)
{
	UNREFERENCED_PARAMETER(DataBuffer);

	NTSTATUS						status = STATUS_INVALID_ADDRESS;
	HID_XFER_PACKET					transferPacket;

	if(DeviceContext->VhfHandle == NULL)
		return STATUS_INVALID_HANDLE;
	
	DeviceContext->HidInputReport.Input.KeyboardInput.Key = 0x04 & 0x00FF;

	transferPacket.reportBufferLen = sizeof(HID_INPUT_REPORT);
	transferPacket.reportBuffer = (UCHAR*)&DeviceContext->HidInputReport;
	transferPacket.reportId = REPORT_ID_KEYBOARD;

	KdPrint(("Trying to submit report"));
	
	status = VhfReadReportSubmit(DeviceContext->VhfHandle, &transferPacket);
	if (NT_SUCCESS(status))
		return status;

	return STATUS_INVALID_PARAMETER;
}