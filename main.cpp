/**
 * oniFixer is a small program to fix the ONI file that don't have required properties for NiTE.
 * Please see https://github.com/VIML/oniFixer for more detail
 *
 * http://viml.nchc.org.tw/home/
 */

#pragma region Header Files
// STL Header
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// OpenNI Header
#include <OpenNI.h>
#include <PS1080.h>
#pragma endregion

using namespace std;
using namespace openni;

#define GET_VIRTUAL_STREAM_IMAGE	100000
#define SET_VIRTUAL_STREAM_IMAGE	100001

// The class hanlde OpenNI Device, VideoStream
class CNIDevice
{
public:
	bool OpenDevice( const char* sURI, bool bDepthOnly = false )
	{
		if( sURI == NULL )
			cout << "Try to open any device" << endl;
		else
			cout << "Open the Device: " << sURI << endl;

		if( mDeivce.open( sURI ) != STATUS_OK )
		{
			cerr << "Can't open source file: " << OpenNI::getExtendedError() << endl;
			return false;
		}

		// Create depth stream
		if( mDeivce.hasSensor( SENSOR_DEPTH ) )
		{
			if( vsDepth.create( mDeivce, SENSOR_DEPTH ) != STATUS_OK )
			{
				cerr << "Can't create depth stream: " << OpenNI::getExtendedError() << endl;
				return false;
			}
		}
		else
		{
			cerr << "ERROR: This source file does not have depth sensor" << endl;
			return false;
		}

		// create color stream, optional
		if( !bDepthOnly && mDeivce.hasSensor( SENSOR_COLOR ) )
		{
			if( vsColor.create( mDeivce, SENSOR_COLOR ) != STATUS_OK )
			{
				cerr << "Can't create color stream: " << OpenNI::getExtendedError() << endl;
				return false;
			}
		}
		return true;
	}

	void Close()
	{
		if( vsDepth.isValid() )
			vsDepth.destroy();

		if( vsColor.isValid() )
			vsColor.destroy();

		mDeivce.close();
	}

	void Start()
	{
		if( vsDepth.isValid() )
			vsDepth.start();

		if( vsColor.isValid() )
			vsColor.start();
	}

	void Stop()
	{
		if( vsDepth.isValid() )
			vsDepth.stop();

		if( vsColor.isValid() )
			vsColor.stop();
	}

public:
	Device		mDeivce;
	VideoStream	vsDepth;
	VideoStream	vsColor;
};

// the NewFrameListener
class CFrameCopy : public VideoStream::NewFrameListener
{
public:
	CFrameCopy( VideoStream& rStream ) : m_rVStream( rStream )
	{
	}

	void onNewFrame( openni::VideoStream& rStream )
	{
		openni::VideoFrameRef mFrame;
		// read frame from real video stream
		if( rStream.readFrame( &mFrame ) == openni::STATUS_OK )
		{
			// get a frame form virtual video stream
			OniFrame* pFrame = NULL;
			if( m_rVStream.invoke( GET_VIRTUAL_STREAM_IMAGE, pFrame ) == openni::STATUS_OK )
			{
				memcpy( pFrame->data, mFrame.getData(), mFrame.getDataSize() );

				pFrame->frameIndex = mFrame.getFrameIndex();
				pFrame->timestamp = mFrame.getTimestamp();

				// write data to form virtual video stream
				m_rVStream.invoke( SET_VIRTUAL_STREAM_IMAGE, pFrame );
			}
		}
		mFrame.release();
	}

protected:
	openni::VideoStream&	m_rVStream;

	CFrameCopy& operator=(const CFrameCopy&);
};

// The class used to process the properties that NiTE require
class CNiTEProperty
{
public:
	CNiTEProperty()
	{
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_CONST_SHIFT			, 8		, "XN_STREAM_PROPERTY_CONST_SHIFT"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_PARAM_COEFF			, 8		, "XN_STREAM_PROPERTY_PARAM_COEFF"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_SHIFT_SCALE			, 8		, "XN_STREAM_PROPERTY_SHIFT_SCALE"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_MAX_SHIFT				, 8		, "XN_STREAM_PROPERTY_MAX_SHIFT"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_S2D_TABLE				, 4096	, "XN_STREAM_PROPERTY_S2D_TABLE"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_D2S_TABLE				, 20002	, "XN_STREAM_PROPERTY_D2S_TABLE"				) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE	, 8		, "XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE"		) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE	, 8		, "XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE"	) );
		vProperties.push_back( CProperty( XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE	, 8		, "XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE"	) );
	}

	bool LoadProperties( const VideoStream& rStream, bool bOverwrite = false )
	{
		for_each( vProperties.begin(), vProperties.end(), [&rStream,&bOverwrite]( CProperty& rProp ){
			if( bOverwrite || !rProp.bLoad )
			{
				cout << "Try load " << rProp.sName << "\t-> ";
				int iSize = rProp.vData.size();
				if( rStream.getProperty( rProp.iIdx, rProp.vData.data(), &iSize ) == STATUS_OK )
				{
					rProp.bLoad = true;
					cout << "OK";
				}
				else
				{
					cout << "Fail";
				}
				cout << endl;
			}
		} );

		return all_of( vProperties.begin(), vProperties.end(), []( CProperty& rProp ){
			return rProp.bLoad;
		} );
	}

	void WriteProperties( VideoStream& rStream )
	{
		for_each( vProperties.begin(), vProperties.end(), [&rStream]( CProperty& rProp ){
			int iSize = rProp.vData.size();
			if( rStream.setProperty( rProp.iIdx, rProp.vData.data(), iSize ) != STATUS_OK )
			{
				cerr << "Property " << rProp.sName << " write fail" << endl;
			}
		} );
	}

protected:
	class CProperty
	{
	public:
		CProperty(){};
		CProperty( int idx, int size, string name )
		{
			iIdx	= idx;
			sName	= name;
			bLoad	= false;
			vData.resize(size);
		}
	
		int		iIdx;
		string	sName;
		bool	bLoad;
		vector<char>	vData;
	};

protected:
	vector<CProperty>	vProperties;
};

// Copy basic properties between VideoStream
void CopyGeneralProperties( const VideoStream& rSource, VideoStream& rTarget )
{
	rTarget.setVideoMode( rSource.getVideoMode() );

	// assign basic properties
	rTarget.setProperty( ONI_STREAM_PROPERTY_VERTICAL_FOV,		rSource.getVerticalFieldOfView() );
	rTarget.setProperty( ONI_STREAM_PROPERTY_HORIZONTAL_FOV,	rSource.getHorizontalFieldOfView() );
	rTarget.setProperty( ONI_STREAM_PROPERTY_MIRRORING,			rSource.getMirroringEnabled() );

	// assign dpeth only properties
	rTarget.setProperty( ONI_STREAM_PROPERTY_MIN_VALUE,			rSource.getMinPixelValue() );
	rTarget.setProperty( ONI_STREAM_PROPERTY_MAX_VALUE,			rSource.getMaxPixelValue() );
}

int main( int argc, char** argv )
{
	std::string	sSource, sTarget;
	#pragma region check input parameters
	if( argc == 3 )
	{
		sSource = argv[1];
		sTarget = argv[2];
	}
	else
	{
		cerr << "Usage: oniFixer source.oni target.oni\n" << endl;
	}
	#pragma endregion

	// Initial OpenNI 
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	CNiTEProperty	mNiTEProp;

	#pragma region Open and check source file
	CNIDevice	mOniFile;
	if( !mOniFile.OpenDevice( sSource.c_str() ) )
	{
		cerr << "Can't open ONI file" << endl;
		return -1;
	}

	// create Playback controller
	PlaybackControl* pPlay = mOniFile.mDeivce.getPlaybackControl();
	int	iDepthFrameNum, iColorFrameNum;
	if( pPlay != nullptr )
	{
		pPlay->setRepeatEnabled( false );
		pPlay->setSpeed( 0.0f );

		iDepthFrameNum = pPlay->getNumberOfFrames( mOniFile.vsDepth );
		iColorFrameNum = pPlay->getNumberOfFrames( mOniFile.vsColor );
		cout << "This oni file have: " << iDepthFrameNum << " frames dpeth, " << iColorFrameNum << " frames color" << endl;
	}
	else
	{
		cerr << "Can't get playback controller, this may not a vaild oni file?" << endl;
		return -1;
	}

	// check NiTE required property
	if( mNiTEProp.LoadProperties( mOniFile.vsDepth ) )
	{
		cout << "All required properties works, don't need to fix" << endl;
		return 0;
	}
	#pragma endregion

	#pragma region Create a physical device to read property
	cout << "\nOpen a physical device to read property" << endl;
	CNIDevice mPhysical;
	if( !mPhysical.OpenDevice( ANY_DEVICE, true ) )
	{
		cerr << "Can't open physical device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// try to load property
	if( !mNiTEProp.LoadProperties( mPhysical.vsDepth ) )
	{
		cout << "Can't prepare all required properties" << endl;
		return -1;
	}
	mPhysical.Close();
	#pragma endregion
	
	#pragma region Create a virtual device to record
	// create recorder
	Recorder mRecorder;
	mRecorder.create( sTarget.c_str() );

	// create virtual device
	cout << "\nCreate a virtual device to record" << endl;
	CNIDevice mVirtual;
	if( !mVirtual.OpenDevice( "\\OpenNI2\\VirtualDevice\\PS1080", !(mOniFile.vsColor.isValid()) ) )
	{
		cerr << "Can't create virtual device" << endl;
		return -1;
	}

	// copy depth stream properties
	CopyGeneralProperties( mOniFile.vsDepth, mVirtual.vsDepth );
	mNiTEProp.WriteProperties( mVirtual.vsDepth );
	mOniFile.vsDepth.addNewFrameListener( new CFrameCopy( mVirtual.vsDepth ) );
	mRecorder.attach( mVirtual.vsDepth );

	// copy color stream properties
	if( mVirtual.vsColor.isValid() )
	{
		CopyGeneralProperties( mOniFile.vsColor, mVirtual.vsColor );
		mOniFile.vsColor.addNewFrameListener( new CFrameCopy( mVirtual.vsColor ) );
		mRecorder.attach( mVirtual.vsColor );
	}
	#pragma endregion

	// start
	cout << "Start to re-recorder\n";
	mRecorder.start();
	mVirtual.Start();
	mOniFile.Start();

	int iDepth = 0;
	while( true )
	{
		// just seek by depth videostream
		pPlay->seek( mOniFile.vsDepth, ++iDepth );

		cout << "." << flush;
		if( iDepth >= iDepthFrameNum )
			break;
	}
	cout << "Done" << endl;
	
	// stop
	mRecorder.stop();
	mOniFile.Stop();
	mVirtual.Stop();

	mOniFile.Close();
	mVirtual.Close();

	OpenNI::shutdown();
}
