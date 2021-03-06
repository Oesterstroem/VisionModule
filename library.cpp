/*****************************************************************************
*                                                                            *
*  OpenNI 1.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>
#include "library.h"
#include <iostream>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define SAMPLE_XML_PATH "../Config/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#define CHECK_RC(rc, what) if(rc != XN_STATUS_OK){printf("%s failed: %s\n", what, xnGetStatusString(rc)); return rc;}
//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------
xn::UserGenerator g_UserGenerator;
XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";	

using namespace xn;
XnBool fileExists(const char *fn)
{
	XnBool exists;
	xnOSDoesFileExist(fn, &exists);
	return exists;
}

//***************************************************************************
// Skeleton function
//***************************************************************************
// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/)
{
    XnUInt32 epochTime = 0;
    xnOSGetEpochTime(&epochTime);
    printf("%d New User %d\n", epochTime, nId);
    // New user found
    if (g_bNeedPose)
    {
        g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
    }
    else
    {
        g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/)
{
    XnUInt32 epochTime = 0;
    xnOSGetEpochTime(&epochTime);
    printf("%d Lost user %d\n", epochTime, nId);	
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(PoseDetectionCapability& /*capability*/, const XnChar* strPose, XnUserID nId, void* /*pCookie*/)
{
    XnUInt32 epochTime = 0;
    xnOSGetEpochTime(&epochTime);
    printf("%d Pose %s detected for user %d\n", epochTime, strPose, nId);
    g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
    g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(SkeletonCapability& /*capability*/, XnUserID nId, void* /*pCookie*/)
{
    XnUInt32 epochTime = 0;
    xnOSGetEpochTime(&epochTime);
    printf("%d Calibration started for user %d\n", epochTime, nId);
}

void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(SkeletonCapability& /*capability*/, XnUserID nId, XnCalibrationStatus eStatus, void* /*pCookie*/)
{
    XnUInt32 epochTime = 0;
    xnOSGetEpochTime(&epochTime);
    if (eStatus == XN_CALIBRATION_STATUS_OK)
    {
        // Calibration succeeded
        printf("%d Calibration complete, start tracking user %d\n", epochTime, nId);		
        g_UserGenerator.GetSkeletonCap().StartTracking(nId);
    }
    else
    {
        // Calibration failed
        printf("%d Calibration failed for user %d\n", epochTime, nId);
        if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
        {
            printf("Manual abort occured, stop attempting to calibrate!");
            return;
        }
        if (g_bNeedPose)
        {
            g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
        }
        else
        {
            g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
        }
    }
}

//***************************************************************************
// Initialize Depth
//***************************************************************************
int libr::initDepth()
{
	libr::DepthClose();
	libr::SkeletonClose();
	nRetVal = XN_STATUS_OK; //Moved from library.h 
	
	nRetVal = context.Init();
	CHECK_RC(nRetVal, "Initialize context");

	nRetVal = depth.Create(context);
	CHECK_RC(nRetVal, "Create depth generator");

	nRetVal = context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGeneratingAll");

	return 0;
}

//***************************************************************************
// Initialize Skeleton
//***************************************************************************

int libr::initSkeleton()
{
	libr::DepthClose();
        libr::SkeletonClose();

nRetVal = XN_STATUS_OK; //Moved from library.h 

	const char *fn = NULL;
	if	(fileExists(SAMPLE_XML_PATH)) fn = SAMPLE_XML_PATH;
	else if (fileExists(SAMPLE_XML_PATH_LOCAL)) fn = 
SAMPLE_XML_PATH_LOCAL;
	else {
		printf("Could not find '%s' nor '%s'. Aborting.\n" , 
SAMPLE_XML_PATH, SAMPLE_XML_PATH_LOCAL);
		return XN_STATUS_ERROR;
	}
	printf("Reading config from: '%s'\n", fn);
	nRetVal = g_Context.InitFromXmlFile(fn, g_scriptNode, &errors);

	if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
	{
		XnChar strError[1024];
		errors.ToString(strError, 1024);
		printf("%s\n", strError);
		return (nRetVal);
	}
	else if (nRetVal != XN_STATUS_OK)
	{
		printf("Open failed: %s\n", xnGetStatusString(nRetVal));
		return (nRetVal);
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, 
g_UserGenerator);
   	 if (nRetVal != XN_STATUS_OK)
   	 {
       		 nRetVal = g_UserGenerator.Create(g_Context);
       		 CHECK_RC(nRetVal, "Find user generator");
   	 }
	 XnCallbackHandle hUserCallbacks, hCalibrationStart, 
hCalibrationComplete, hPoseDetected;
   	if 
(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
    	{
        printf("Supplied user generator doesn't support skeleton\n");
        return 1;
    	}
    	nRetVal = g_UserGenerator.RegisterUserCallbacks(User_NewUser, 
User_LostUser, NULL, hUserCallbacks);
    	CHECK_RC(nRetVal, "Register to user callbacks");
    	nRetVal = 
g_UserGenerator.GetSkeletonCap().RegisterToCalibrationStart(UserCalibration_CalibrationStart, 
NULL, hCalibrationStart);
    	CHECK_RC(nRetVal, "Register to calibration start");
    	nRetVal = 
g_UserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(UserCalibration_CalibrationComplete, 
NULL, hCalibrationComplete);
    	CHECK_RC(nRetVal, "Register to calibration complete");

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
    	{
        	g_bNeedPose = TRUE;
        	if 
(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
        	{
            		printf("Pose required, but not supported\n");
            		return 1;
       	 	}
        	nRetVal = 
g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(UserPose_PoseDetected, 
NULL, hPoseDetected);
        	CHECK_RC(nRetVal, "Register to Pose Detected");
        	
g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
    	}

    	
g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

    	nRetVal = g_Context.StartGeneratingAll();
    	CHECK_RC(nRetVal, "StartGenerating");
 
	return 0;
}

//***************************************************************************
// Skeleton
//***************************************************************************

//************ Head Position *************************************
int libr::SkeletonHeadposition(){
	
	g_Context.WaitOneUpdateAll(g_UserGenerator);
        // print the torso information for the first user already tracking
        nUsers=MAX_NUM_USERS;
        g_UserGenerator.GetUsers(aUsers, nUsers);
        
	for(XnUInt16 i=0; i<nUsers; i++)
        {
            	if(g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])==FALSE){
		output_int[i] = aUsers[i];
	   	output_float[i][0] = 0;
	   	output_float[i][1] = 0;
		output_float[i][2] = 0;
                continue;
		}

            	g_UserGenerator.GetSkeletonCap().GetSkeletonJoint(aUsers[i],XN_SKEL_TORSO,torsoJoint);
           	output_int[i] = aUsers[i];
	   	output_float[i][0] = torsoJoint.position.position.X;
	   	output_float[i][1] = torsoJoint.position.position.Y;
		output_float[i][2] = torsoJoint.position.position.Z;
	   
	    //printf("user %d: head at (%6.2f,%6.2f,%6.2f)\n" , aUsers[i] , torsoJoint.position.position.X , torsoJoint.position.position.Y , torsoJoint.position.position.Z);
        }     
}

//***************************************************************************
// Depth
//***************************************************************************

//************ Middle *************************************
int libr::DepthMiddle()
{
	nRetVal = context.WaitOneUpdateAll(depth);//update depth
	if (nRetVal != XN_STATUS_OK)
	{
		printf("UpdateData failed: %s\n", xnGetStatusString(nRetVal));
	}

	depth.GetMetaData(depthMD); //get depthdata
	FrameID = depthMD.FrameID(); //Storing frame number
	output_int[0] = depthMD(depthMD.XRes()/2,depthMD.YRes()/2); //Storing output

	return 0;
}


//************ Point *************************************
int libr::DepthPoint(int x , int y)
{
	nRetVal = context.WaitOneUpdateAll(depth);
	if (nRetVal != XN_STATUS_OK)
	{
		printf("UpdateData failed: %s\n", xnGetStatusString(nRetVal));
	}

	depth.GetMetaData(depthMD);
	FrameID = depthMD.FrameID(); //Storing frame number
	output_int[0] = depthMD(x,y); //Storing output
		
//printf("Frame %d Middle point is: %u. FPS: %f\n", depthMD.FrameID(), depthMD(depthMD.XRes() / 2, depthMD.YRes() / 2), xnFPSCalc(&xnFPS));
	


	return 0;
}

//************ Resolution *************************************
int libr::DepthResolution()
{
	nRetVal = context.WaitOneUpdateAll(depth);
	if (nRetVal != XN_STATUS_OK)
	{
		printf("UpdateData failed: %s\n", xnGetStatusString(nRetVal));
	}

	depth.GetMetaData(depthMD);
	FrameID = depthMD.FrameID(); //Storing frame number
	output_int[0] = depthMD.XRes(); //Storing output
	output_int[1] = depthMD.YRes(); //Storing output
		
//printf("Frame %d Middle point is: %u. FPS: %f\n", depthMD.FrameID(), depthMD(depthMD.XRes() / 2, depthMD.YRes() / 2), xnFPSCalc(&xnFPS));
	


	return 0;
}
//***************************************************************************
// Close stream
//***************************************************************************
int libr::DepthClose()
{
	depth.Release();
	scriptNode.Release();
	context.Release();

	return 0;
}

int libr::SkeletonClose()
{
	g_scriptNode.Release();
    	g_UserGenerator.Release();
   	g_Context.Release();

	return 0;
}
