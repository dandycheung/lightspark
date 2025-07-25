/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "platforms/engineutils.h"
#include "scripting/abc.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/class.h"
#include "compat.h"
#include <iostream>
#include "backends/audio.h"
#include "backends/rendering.h"
#include "backends/cachedsurface.h"
#include "backends/streamcache.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/display/RootMovieClip.h"
#include <unistd.h>

using namespace lightspark;
using namespace std;

SoundTransform::SoundTransform(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_SOUNDTRANSFORM),volume(100),leftToLeft(100),leftToRight(0),rightToLeft(0),rightToRight(100)
{
}

void SoundTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("pan","",c->getSystemState()->getBuiltinFunction(_get_pan,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("pan","",c->getSystemState()->getBuiltinFunction(_set_pan),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("volume","",c->getSystemState()->getBuiltinFunction(_get_volume,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("volume","",c->getSystemState()->getBuiltinFunction(_set_volume),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("leftToLeft","",c->getSystemState()->getBuiltinFunction(_get_leftToLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("leftToLeft","",c->getSystemState()->getBuiltinFunction(_set_leftToLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("leftToRight","",c->getSystemState()->getBuiltinFunction(_get_leftToRight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("leftToRight","",c->getSystemState()->getBuiltinFunction(_set_leftToRight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rightToLeft","",c->getSystemState()->getBuiltinFunction(_get_rightToLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rightToLeft","",c->getSystemState()->getBuiltinFunction(_set_rightToLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rightToRight","",c->getSystemState()->getBuiltinFunction(_get_rightToRight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rightToRight","",c->getSystemState()->getBuiltinFunction(_set_rightToRight),SETTER_METHOD,true);
}

bool SoundTransform::destruct()
{
	volume=100;
	leftToLeft=100;
	leftToRight=0;
	rightToLeft=0;
	rightToRight=100;
	return ASObject::destruct();
}

void SoundTransform::setPan(number_t pan)
{
	if (pan >= 0)
	{
		leftToLeft = 100-pan*100.0;
		rightToRight = 100;
	}
	else
	{
		leftToLeft = 100;
		rightToRight = 100+pan*100.0;
	}
	leftToRight = 0;
	rightToLeft = 0;
}

ASFUNCTIONBODY_ATOM(SoundTransform,_constructor)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t pan;
	number_t volume;
	ARG_CHECK(ARG_UNPACK(volume, 1.0)(pan, 0.0));
	th->volume = volume*100.0;
	th->setPan(pan);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_get_pan)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	if (th->leftToRight != 0.0 || th->rightToLeft != 0.0)
		asAtomHandler::setNumber(ret,wrk,0.0);
	else
	{
		number_t n = number_t(th->leftToLeft/100.0);
		asAtomHandler::setNumber(ret,wrk,1.0-n*n);
	}
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_pan)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t pan;
	ARG_CHECK(ARG_UNPACK(pan));
	th->setPan(pan);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_get_volume)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t n = number_t(th->volume)/100.0;
	asAtomHandler::setNumber(ret,wrk,n);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_volume)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t volume;
	ARG_CHECK(ARG_UNPACK(volume));
	th->volume = volume*100.0;
}

ASFUNCTIONBODY_ATOM(SoundTransform,_get_leftToLeft)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	ret = asAtomHandler::fromNumber(wrk,number_t(th->leftToLeft)/100.0,false);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_leftToLeft)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t leftToLeft;
	ARG_CHECK(ARG_UNPACK(leftToLeft));
	th->leftToLeft = leftToLeft*100.0;
}
ASFUNCTIONBODY_ATOM(SoundTransform,_get_leftToRight)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	ret = asAtomHandler::fromNumber(wrk,number_t(th->leftToRight)/100.0,false);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_leftToRight)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t leftToRight;
	ARG_CHECK(ARG_UNPACK(leftToRight));
	th->leftToRight = leftToRight*100.0;
}
ASFUNCTIONBODY_ATOM(SoundTransform,_get_rightToLeft)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	ret = asAtomHandler::fromNumber(wrk,number_t(th->rightToLeft)/100.0,false);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_rightToLeft)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t rightToLeft;
	ARG_CHECK(ARG_UNPACK(rightToLeft));
	th->rightToLeft = rightToLeft*100.0;
}
ASFUNCTIONBODY_ATOM(SoundTransform,_get_rightToRight)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	ret = asAtomHandler::fromNumber(wrk,number_t(th->rightToRight)/100.0,false);
}
ASFUNCTIONBODY_ATOM(SoundTransform,_set_rightToRight)
{
	SoundTransform* th=asAtomHandler::as<SoundTransform>(obj);
	number_t rightToRight;
	ARG_CHECK(ARG_UNPACK(rightToRight));
	th->rightToRight = rightToRight*100.0;
}

void Video::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("videoWidth","",c->getSystemState()->getBuiltinFunction(_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("videoHeight","",c->getSystemState()->getBuiltinFunction(_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(Video::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(Video::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(Video::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(Video::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",c->getSystemState()->getBuiltinFunction(attachNetStream),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, deblocking);
	REGISTER_GETTER_SETTER(c, smoothing);
}

ASFUNCTIONBODY_GETTER_SETTER(Video, deblocking)
ASFUNCTIONBODY_GETTER_SETTER(Video, smoothing)

bool Video::destruct()
{
	videotag=nullptr;
	resetDecoder();
	netStream.reset();
	return DisplayObject::destruct();
}

void Video::finalize()
{
	resetDecoder();
	netStream.reset();
	DisplayObject::finalize();
}

void Video::advanceFrame(bool implicit)
{
	if (!isRendering)
		this->requestInvalidation(getSystemState());
}

void Video::refreshSurfaceState()
{
	DisplayObject::refreshSurfaceState();
	if (!isRendering)
	{
		Locker l(mutex);
		if (netStream && netStream->lockIfReady())
		{
			this->getCachedSurface()->tex = &netStream->getTexture();
			//Get size
			videoWidth=netStream->getVideoWidth();
			videoHeight=netStream->getVideoHeight();
			isRendering=true;
			netStream->unlock();
		}
		if (embeddedVideoDecoder)
		{
			this->getCachedSurface()->tex = &embeddedVideoDecoder->getTexture();
			videoWidth=videotag->Width;
			videoHeight=videotag->Height;
			isRendering=true;
		}
		this->getCachedSurface()->isChunkOwner=false;
	}
	else
	{
		Locker l(mutex);
		if (!netStream && !embeddedVideoDecoder)
		{
			this->getCachedSurface()->tex = nullptr;
			this->getCachedSurface()->isChunkOwner=false;
			isRendering=false;
		}
	}
	this->getCachedSurface()->getState()->matrix = MATRIX();
}
void Video::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	hasChanged=true;
	incRef();
	q->addToInvalidateQueue(_MR(this));
}
IDrawable *Video::invalidate(bool smoothing)
{
	number_t x,y;
	number_t width,height;
	number_t bxmin=0,bxmax=0,bymin=0,bymax=0;
	boundsRect(bxmin,bxmax,bymin,bymax,false);
	MATRIX matrix = getMatrix();
	
	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);
	
	ColorTransformBase ct;
	if (this->colorTransform)
		ct=*this->colorTransform.getPtr();
	
	this->resetNeedsTextureRecalculation();
	IDrawable* res = new RefreshableDrawable(x, y, ceil(width), ceil(height)
											 , matrix.getScaleX(), matrix.getScaleY()
											 , isMask, cacheAsBitmap
											 , getScaleFactor(),getConcatenatedAlpha()
											 , ct, smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
	res->getState()->isYUV=true;
	return res;
}

void Video::resetDecoder()
{
	Locker l(mutex);
	isRendering=false;
	lastuploadedframe=UINT32_MAX;
	if (embeddedVideoDecoder)
	{
		if (embeddedVideoDecoder->isUploading())
			embeddedVideoDecoder->markForDestruction();
		else
			delete embeddedVideoDecoder;
		embeddedVideoDecoder=nullptr;
	}
}

Video::Video(ASWorker* wk, Class_base* c, uint32_t w, uint32_t h, DefineVideoStreamTag *v)
	: DisplayObject(wk,c),width(w),height(h),videoWidth(0),videoHeight(0),
	netStream(NullRef),videotag(v),embeddedVideoDecoder(nullptr),lastuploadedframe(UINT32_MAX),isRendering(false),
	deblocking(v ? v->VideoFlagsDeblocking:0),smoothing(v ? v->VideoFlagsSmoothing : false)
{
	subtype=SUBTYPE_VIDEO;
	if (videotag)
	{
		videoWidth=width=videotag->Width;
		videoHeight=height=videotag->Height;
	}
}

void Video::checkRatio(uint32_t ratio, bool inskipping)
{
	Locker l(mutex);
	if (videotag)
	{
#ifdef ENABLE_LIBAVCODEC
		if (embeddedVideoDecoder==nullptr)
		{
			LS_VIDEO_CODEC lscodec;
			bool ok=false;
			switch (videotag->VideoCodecID)
			{
				case 2:
					lscodec = LS_VIDEO_CODEC::H263;
					ok=true;
					break;
				case 3:
					LOG(LOG_ERROR,"video codec SCREEN not implemented for embedded video");
					break;
				case 4:
					lscodec = LS_VIDEO_CODEC::VP6;
					ok=true;
					break;
				case 5:
					lscodec = LS_VIDEO_CODEC::VP6A;
					ok=true;
					break;
				default:
					LOG(LOG_ERROR,"invalid video codec id for embedded video:"<<int(videotag->VideoCodecID));
					break;
			}
			if (ok)
			{
				embeddedVideoDecoder = new FFMpegVideoDecoder(lscodec,nullptr,0,videotag->loadedFrom->getFrameRate(),videotag);
				this->requestInvalidation(getSystemState());
			}
			lastuploadedframe=UINT32_MAX;
		}
#endif
		if (embeddedVideoDecoder && !embeddedVideoDecoder->isUploading() && videotag->NumFrames > 0)
		{
			ratio %= videotag->NumFrames;
			if (ratio != lastuploadedframe && videotag->frames[ratio])
			{
				embeddedVideoDecoder->waitForFencing();
				embeddedVideoDecoder->setVideoFrameToDecode(ratio);
				lastuploadedframe = ratio;
				getSystemState()->getRenderThread()->addUploadJob(embeddedVideoDecoder);
			}
		}
	}
}

void Video::afterLegacyInsert()
{
	if(!getConstructIndicator() && !needsActionScript3())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		getClass()->handleConstruction(obj,nullptr,0,true);
	}
	DisplayObject::afterLegacyInsert();
}

void Video::afterLegacyDelete(bool inskipping)
{
	Locker l(mutex);
	resetDecoder();
}

void Video::setOnStage(bool staged, bool force,bool inskipping)
{
	if(staged!=onStage||force)
	{
		if (!staged)
		{
			Locker l(mutex);
			resetDecoder();
		}
	}
	DisplayObject::setOnStage(staged,force,inskipping);
}

uint32_t Video::getTagID() const
{
	return videotag ? uint32_t(videotag->CharacterID) : UINT32_MAX;
}

Video::~Video()
{
}

bool Video::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

ASFUNCTIONBODY_ATOM(Video,_constructor)
{
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen<=2);
	if(0 < argslen)
		th->width=asAtomHandler::toInt(args[0]);
	if(1 < argslen)
		th->height=asAtomHandler::toInt(args[1]);
}

ASFUNCTIONBODY_ATOM(Video,_getVideoWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	if (th->videoWidth == 0 && !th->netStream.isNull() && th->netStream->lockIfReady())
	{
		//Get size
		th->videoWidth=th->netStream->getVideoWidth();
		th->videoHeight=th->netStream->getVideoHeight();
		th->netStream->unlock();
	}
	asAtomHandler::setUInt(ret,wrk,th->videoWidth);
}

ASFUNCTIONBODY_ATOM(Video,_getVideoHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	if (th->videoHeight == 0 && !th->netStream.isNull() && th->netStream->lockIfReady())
	{
		//Get size
		th->videoWidth=th->netStream->getVideoWidth();
		th->videoHeight=th->netStream->getVideoHeight();
		th->netStream->unlock();
	}
	asAtomHandler::setUInt(ret,wrk,th->videoHeight);
}

ASFUNCTIONBODY_ATOM(Video,_getWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,wrk,th->width);
}

ASFUNCTIONBODY_ATOM(Video,_setWidth)
{
	Video* th=asAtomHandler::as<Video>(obj);
	Locker l(th->mutex);
	assert_and_throw(argslen==1);
	th->width=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(Video,_getHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	asAtomHandler::setUInt(ret,wrk,th->height);
}

ASFUNCTIONBODY_ATOM(Video,_setHeight)
{
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen==1);
	Locker l(th->mutex);
	th->height=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(Video,attachNetStream)
{
	Video* th=asAtomHandler::as<Video>(obj);
	assert_and_throw(argslen==1);
	th->resetDecoder();
	if(asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0])) //Drop the connection
	{
		Locker l(th->mutex);
		th->netStream=NullRef;
		th->requestInvalidation(wrk->getSystemState());
		return;
	}

	//Validate the parameter
	if(!asAtomHandler::is<NetStream>(args[0]))
		throw RunTimeException("Type mismatch in Video::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Locker l(th->mutex);
	th->netStream=_MR(asAtomHandler::as<NetStream>(args[0]));
	th->requestInvalidation(wrk->getSystemState());
}
ASFUNCTIONBODY_ATOM(Video,clear)
{
	Video* th=asAtomHandler::as<Video>(obj);
	if (th->embeddedVideoDecoder)
		th->embeddedVideoDecoder->clearFrameBuffer();
	if (th->netStream)
		th->netStream->clearFrameBuffer();
}

_NR<DisplayObject> Video::hitTestImpl(const Vector2f&, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	//TODO: support masks
	//TODO: Add a point intersect function to RECT, and use that instead.
	if(localPoint.x>=0 && localPoint.x<=width && localPoint.y>=0 && localPoint.y<=height)
	{
		this->incRef();
		return _MR(this);
	}
	else
		return NullRef;
}

Sound::Sound(ASWorker* wrk, Class_base* c)
	:EventDispatcher(wrk,c),downloader(nullptr),soundData(nullptr),soundChannel(nullptr),rawDataStreamDecoder(nullptr),rawDataStartPosition(0),rawDataStreamBuf(nullptr),rawDataStream(nullptr),buffertime(1000),
	 container(true),sampledataprocessed(true),format(CODEC_NONE, 0, 0),bytesLoaded(0),bytesTotal(0),length(-1)
{
	subtype=SUBTYPE_SOUND;
}

Sound::Sound(ASWorker* wrk,Class_base* c, _R<StreamCache> data, AudioFormat _format, number_t duration_in_ms)
	:EventDispatcher(wrk,c),downloader(nullptr),soundData(data),soundChannel(nullptr),rawDataStreamDecoder(nullptr),rawDataStartPosition(0),rawDataStreamBuf(nullptr),rawDataStream(nullptr),buffertime(1000),
	 container(false),sampledataprocessed(true),format(_format),
	 bytesLoaded(soundData->getReceivedLength()),
	 bytesTotal(soundData->getReceivedLength()),length(duration_in_ms)
{
	subtype=SUBTYPE_SOUND;
}

Sound::~Sound()
{
}

void Sound::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("load","",c->getSystemState()->getBuiltinFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play,0,Class<SoundChannel>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("extract","",c->getSystemState()->getBuiltinFunction(extract,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadCompressedDataFromByteArray","",c->getSystemState()->getBuiltinFunction(loadCompressedDataFromByteArray),NORMAL_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,bytesLoaded,UInteger);
	REGISTER_GETTER_RESULTTYPE(c,bytesTotal,UInteger);
	REGISTER_GETTER_RESULTTYPE(c,length,Number);
}

void Sound::finalize()
{
	if(downloader && getSystemState()->downloadManager)
		getSystemState()->downloadManager->destroy(downloader);
	if (rawDataStreamDecoder)
		delete rawDataStreamDecoder;
	rawDataStreamDecoder=nullptr;
	if (rawDataStream)
		delete rawDataStream;
	rawDataStream=nullptr;
	if (rawDataStreamBuf)
		delete rawDataStreamBuf;
	rawDataStreamBuf=nullptr;
	if (soundChannel)
		soundChannel->removeStoredMember();
	soundChannel=nullptr;
	EventDispatcher::finalize();
}

bool Sound::destruct()
{
	if(downloader && getSystemState()->downloadManager)
		getSystemState()->downloadManager->destroy(downloader);
	if (rawDataStreamDecoder)
		delete rawDataStreamDecoder;
	rawDataStreamDecoder=nullptr;
	if (rawDataStream)
		delete rawDataStream;
	rawDataStream=nullptr;
	if (rawDataStreamBuf)
		delete rawDataStreamBuf;
	rawDataStreamBuf=nullptr;
	if (soundChannel)
		soundChannel->removeStoredMember();
	soundChannel=nullptr;
	return EventDispatcher::destruct();
}

void Sound::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (soundChannel)
		soundChannel->prepareShutdown();
}

bool Sound::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (soundChannel)
		ret = soundChannel->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_ATOM(Sound,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
	if (argslen>0)
		Sound::load(ret,wrk,obj, args, argslen);
}

ASFUNCTIONBODY_ATOM(Sound,load)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	_NR<URLRequest> urlRequest;
	_NR<SoundLoaderContext> context;
	
	ARG_CHECK(ARG_UNPACK(urlRequest)(context,NullRef));
	if (!urlRequest.isNull())
	{
		th->url = urlRequest->getRequestURL();
		urlRequest->getPostData(th->postData);
	}
	if (!context.isNull())
		th->buffertime = context->bufferTime;
	if (urlRequest.isNull())
		return;

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(wrk)));
		return;
	}
	_R<StreamCache> c(_MR(new MemoryStreamCache(th->getSystemState())));
	th->soundData = c;

	//The URL is valid so we can start the download

	if(th->postData.empty())
	{
		//This is a GET request
		//Use disk cache our downloaded files
		th->incRef();
		th->downloader=th->getSystemState()->downloadManager->download(th->url, th->soundData, th);
	}
	else
	{
		list<tiny_string> headers=urlRequest->getHeaders();
		th->incRef();
		th->downloader=th->getSystemState()->downloadManager->downloadWithData(th->url,
				th->soundData, th->postData, headers, th);
		//Clean up the postData for the next load
		th->postData.clear();
	}
	if(th->downloader->hasFailed())
	{
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(wrk)));
	}
}

ASFUNCTIONBODY_ATOM(Sound,play)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	number_t startTime;
	int32_t loops;
	_NR<SoundTransform> soundtransform;
	ARG_CHECK(ARG_UNPACK(startTime, 0)(loops,0)(soundtransform,NullRef));
	if (!wrk->rootClip->needsActionScript3()) // actionscript2 expects the starttime in seconds, actionscript3 in milliseconds
		startTime *= 1000;
	if (soundtransform.isNull())
		soundtransform = _MR(Class<SoundTransform>::getInstanceSNoArgs(wrk));
	if (th->container)
	{
		if (!wrk->rootClip->needsActionScript3())
		{
			LOG(LOG_ERROR,"playing sound without attached tag, ignored");
			return;
		}
		RELEASE_WRITE(th->sampledataprocessed,true);
		th->setSoundChannel(Class<SoundChannel>::getInstanceS(wrk,::ceil(th->buffertime/1000.0),NullRef, AudioFormat(LINEAR_PCM_FLOAT_PLATFORM_ENDIAN,44100,2),nullptr,th));
		th->soundChannel->setLoops(loops);
		th->soundChannel->soundTransform = soundtransform;
		th->soundChannel->play(startTime);
		th->soundChannel->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(th->soundChannel);
	}
	else
	{
		if (th->soundChannel)
		{
			th->soundChannel->setLoops(loops);
			if (th->is<AVM1Sound>())
				th->soundChannel->setSampleProducer(th);
			th->soundChannel->soundTransform = soundtransform;
			th->soundChannel->play(startTime);
			th->soundChannel->incRef();
			ret = asAtomHandler::fromObjectNoPrimitive(th->soundChannel);
			return;
		}
		SoundChannel* s = Class<SoundChannel>::getInstanceS(wrk,::ceil(th->buffertime/1000.0),th->soundData, th->format);
		s->setStartTime(startTime);
		s->setLoops(loops);
		if (th->is<AVM1Sound>())
			th->soundChannel->setSampleProducer(th);
		s->soundTransform = soundtransform;
		s->play(startTime);
		ret = asAtomHandler::fromObjectNoPrimitive(s);
	}
}

ASFUNCTIONBODY_ATOM(Sound,close)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	if(th->downloader)
	{
		th->downloader->stop();
		th->decRef();
	}
}
ASFUNCTIONBODY_ATOM(Sound,extract)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	_NR<ByteArray> target;
	number_t length;
	int32_t startPosition;
	ARG_CHECK(ARG_UNPACK(target)(length)(startPosition,-1));
	int32_t readcount=0;
	int32_t bytelength=length*4*2; // length is in samples (2 32bit floats)
	int32_t bytestartposition= startPosition*4*2; // startposition is in samples (2 32bit floats)
	if (!target.isNull() && !th->soundData.isNull() && th->soundData->getReceivedLength() > 0)
	{
		int32_t readcount=0;
		try
		{
#ifdef ENABLE_LIBAVCODEC
			if (th->rawDataStreamDecoder && bytestartposition < th->rawDataStartPosition)
			{
				delete th->rawDataStreamDecoder;
				th->rawDataStreamDecoder= nullptr;
				th->rawDataStartPosition=0;
			}
			if (bytestartposition < 0)
				bytestartposition = th->rawDataStartPosition;
			if (th->rawDataStreamDecoder== nullptr)
			{
				if (th->rawDataStream)
					delete th->rawDataStream;
				if (th->rawDataStreamBuf)
					delete th->rawDataStreamBuf;
				th->rawDataStreamBuf = th->soundData->createReader();
				th->rawDataStream = new istream(th->rawDataStreamBuf);
				th->rawDataStream->exceptions ( istream::failbit | istream::badbit );
				th->rawDataStreamDecoder=new FFMpegStreamDecoder(nullptr,wrk->getSystemState()->getEngineData(),*th->rawDataStream,::ceil(th->getBufferTime()/1000.0),&th->format,th->soundData->hasTerminated() ? th->soundData->getReceivedLength() : -1,true);
			}
			if(!th->rawDataStreamDecoder->isValid())
			{
				LOG(LOG_ERROR,"invalid streamDecoder in Sound.extract:"<<th->format.codec<<" "<<th->format.channels<<"/"<<th->format.sampleRate<<" "<<th->soundData->getReceivedLength());
				delete th->rawDataStreamDecoder;
				th->rawDataStreamDecoder=nullptr;
			}
			else if(th->rawDataStreamDecoder->audioDecoder)
			{
				uint8_t* data = new uint8_t[bytelength];
				bool firstcopy=true;
				while(true)
				{
					if (!th->rawDataStreamDecoder->audioDecoder->hasDecodedFrames())
					{
						bool decodingSuccess=th->rawDataStreamDecoder->decodeNextFrame();
						if(decodingSuccess==false)
							break;
					}
					uint8_t buf[MAX_AUDIO_FRAME_SIZE];
					 
					uint32_t read = th->getSystemState()->getEngineData()->audio_useFloatSampleFormat() ?
								th->rawDataStreamDecoder->audioDecoder->copyFrameF32((float *)buf,
																					  min(th->rawDataStartPosition >= bytestartposition
																						  ? bytelength-readcount
																						  : bytestartposition - th->rawDataStartPosition
																							, MAX_AUDIO_FRAME_SIZE)):
								th->rawDataStreamDecoder->audioDecoder->copyFrameS16((int16_t *)buf,
																					  min(th->rawDataStartPosition >= bytestartposition
																						  ? bytelength-readcount
																						  : bytestartposition - th->rawDataStartPosition
																							, MAX_AUDIO_FRAME_SIZE));
					th->rawDataStartPosition += read;
					if (th->rawDataStartPosition > bytestartposition)
					{
						if (firstcopy)
						{
							firstcopy=false;
							int bufstart = read - (th->rawDataStartPosition - bytestartposition);
							read = (th->rawDataStartPosition - bytestartposition);
							memcpy(data+readcount,buf+bufstart,min(read,uint32_t(bytelength-readcount)));
						}
						else
							memcpy(data+readcount,buf,min(read,uint32_t(bytelength-readcount)));
						readcount+=read;
					}
					if (th->rawDataStartPosition-bytestartposition >= bytelength)
						break;
				}
				// ffmpeg always returns decoded data in native endian format, so we have to convert to the target endian setting
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				if (target->getLittleEndian())
				{
					for (int32_t i = 0; i < min(readcount,bytelength); i+=4)
					{
						uint32_t* u = (uint32_t*)(&data[i]);
						*u = LS_UINT32_TO_LE(*u);
					}
				}
#else
				if (!target->getLittleEndian())
				{
					for (int32_t i = 0; i < min(readcount,bytelength); i+=4)
					{
						uint32_t* u = (uint32_t*)(&data[i]);
						*u = LS_UINT32_TO_BE(*u);
					}
				}
#endif
				target->writeBytes(data,min(readcount,bytelength));
				delete[] data;
			}
#endif //ENABLE_LIBAVCODEC
		}
		catch(exception& e)
		{
			LOG(LOG_ERROR, "Exception in extracting sound data: "<<e.what());
		}
	}
	ret = asAtomHandler::fromInt(min(readcount,bytelength)/8);
}

ASFUNCTIONBODY_ATOM(Sound,loadCompressedDataFromByteArray)
{
	Sound* th=asAtomHandler::as<Sound>(obj);
	_NR<ByteArray> bytes;
	uint32_t bytesLength;

	ARG_CHECK(ARG_UNPACK(bytes)(bytesLength));
	_R<StreamCache> c(_MR(new MemoryStreamCache(th->getSystemState())));
	th->soundData = c;
	if (bytes)
	{
		uint8_t* buf = new uint8_t[bytesLength];
		uint32_t oldpos = bytes->getPosition();
		if (bytes->readBytes(oldpos,bytesLength,buf))
		{
			th->soundData->append(buf,bytesLength);
			bytes->setPosition(oldpos+bytesLength);
		}
		delete[] buf;
	}
	th->soundData->markFinished();
	th->container=false;
}
void Sound::afterExecution(_R<Event> e)
{
	if (e->type == "sampleData")
	{
		_NR<ByteArray> data = e->as<SampleDataEvent>()->data;
		this->soundChannel->appendSampleData(data.getPtr());
		RELEASE_WRITE(sampledataprocessed,true);
		this->soundChannel->semSampleData.signal();
	}
}

void Sound::requestSampleDataEvent(size_t position)
{
	RELEASE_WRITE(sampledataprocessed,false);
	// request more data
	_NR<ByteArray> data = _MR(Class<ByteArray>::getInstanceS(getInstanceWorker()));
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	data->setLittleEndian(true);
#endif
	incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<SampleDataEvent>::getInstanceS(getInstanceWorker(),data,position)));
}

void Sound::setBytesTotal(uint32_t b)
{
	bytesTotal=b;
}

void Sound::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		bytesLoaded=b;
		// make sure that the event queue is not flooded with progressEvents
		if (progressEvent.isNull())
		{
			this->incRef();
			progressEvent = _MR(Class<ProgressEvent>::getInstanceS(getInstanceWorker(),bytesLoaded,bytesTotal));
			progressEvent->incRef();
			getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
		}
		else
		{
			// event already exists, we only update the values
			Locker l(progressEvent->accesmutex);
			progressEvent->bytesLoaded = bytesLoaded;
			progressEvent->bytesTotal = bytesTotal;
			// if event is already in event queue, we don't need to add it again
			if (!progressEvent->queued)
			{
				this->incRef();
				getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
			}
		}
		if(bytesLoaded==bytesTotal)
		{
			this->incRef();
			getVm(getSystemState())->addIdleEvent(_MR(this),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"complete")));
		}
	}
}

void Sound::setSoundChannel(SoundChannel* channel)
{
	soundChannel = channel;
	soundChannel->addStoredMember();
}

ASFUNCTIONBODY_GETTER(Sound,bytesLoaded)
ASFUNCTIONBODY_GETTER(Sound,bytesTotal)
ASFUNCTIONBODY_GETTER(Sound,length)

void SoundMixer::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("stopAll","",c->getSystemState()->getBuiltinFunction(stopAll),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("computeSpectrum","",c->getSystemState()->getBuiltinFunction(computeSpectrum),NORMAL_METHOD,false);
	REGISTER_GETTER_SETTER_STATIC(c,bufferTime);
	REGISTER_GETTER_SETTER_STATIC(c,soundTransform);
}
ASFUNCTIONBODY_GETTER_SETTER_STATIC(SoundMixer,bufferTime)
ASFUNCTIONBODY_GETTER_SETTER_STATIC(SoundMixer,soundTransform)

ASFUNCTIONBODY_ATOM(SoundMixer,stopAll)
{
	wrk->getSystemState()->audioManager->stopAllSounds();
	ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(SoundMixer,computeSpectrum)
{
	_NR<ByteArray> output;
	bool FFTMode;
	int stretchFactor;
	ARG_CHECK(ARG_UNPACK (output) (FFTMode,false) (stretchFactor,0));
	output->setLength(0);
	output->setPosition(0);
	for (int i = 0; i < 4*512; i++) // 512 floats
		output->writeByte(0);
	output->setPosition(0);
	LOG(LOG_NOT_IMPLEMENTED,"SoundMixer.computeSpectrum not implemented");
}

void SoundLoaderContext::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,bufferTime);
	REGISTER_GETTER_SETTER(c,checkPolicyFile);
}

ASFUNCTIONBODY_ATOM(SoundLoaderContext,_constructor)
{
	SoundLoaderContext* th=asAtomHandler::as<SoundLoaderContext>(obj);
	ARG_CHECK(ARG_UNPACK(th->bufferTime,1000)(th->checkPolicyFile,false));
}

ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,bufferTime)
ASFUNCTIONBODY_GETTER_SETTER(SoundLoaderContext,checkPolicyFile)

SoundChannel::SoundChannel(ASWorker* wrk, Class_base* c, uint32_t _buffertimeseconds, _NR<StreamCache> _stream, AudioFormat _format, const SOUNDINFO* _soundinfo, Sound* _sampleproducer, bool _forstreaming)
	: EventDispatcher(wrk,c),buffertimeseconds(_buffertimeseconds),stream(_stream),sampleproducer(_sampleproducer),starting(true),stopped(true),terminated(true),stopping(false),finished(false),audioDecoder(nullptr),audioStream(nullptr),
	format(_format),soundinfo(_soundinfo),oldVolume(-1.0),startTime(0),loopstogo(0),streamposition(0),streamdatafinished(false),restartafterabort(false),forstreaming(_forstreaming),fromSoundTag(nullptr),
	leftPeak(1),rightPeak(1),semSampleData(0)
{
	subtype=SUBTYPE_SOUNDCHANNEL;
	if (soundinfo)
		setLoops(soundinfo->LoopCount);
	if (sampleproducer)
	{
		sampleproducer->incRef();
		sampleproducer->addStoredMember();
	}
}

SoundChannel::~SoundChannel()
{
}

void SoundChannel::appendStreamBlock(unsigned char *buf, int len)
{
	if (stream)
		SoundStreamBlockTag::decodeSoundBlock(stream.getPtr(),format.codec,buf,len);
}

void SoundChannel::appendSampleData(ByteArray* data)
{
	if (data == nullptr)
	{
		streamdatafinished=true;
		threadAbort();
	}
	if (!isPlaying() && !isStarting())
		return;
	if (!audioDecoder)
		return;
	uint32_t len = data->getLength();
	uint32_t toprocess=len;
	while (toprocess)
	{
		uint32_t samples = audioDecoder->decodeData(data->getBufferNoCheck(),toprocess,streamposition/(44100*2/1000));
		streamposition+=samples;
		if (toprocess > samples*sizeof(float))
			toprocess -= samples*sizeof(float);
		else
			toprocess=0;
	}
	resume();
	if (len < 2048*sizeof(float))
	{
		// less than 2048 samples, stop requesting data
		streamdatafinished=true;
	}
}

void SoundChannel::play(number_t starttime)
{
	mutex.lock();
	RELEASE_WRITE(starting,true);
	if (!ACQUIRE_READ(stopped))
	{
		RELEASE_WRITE(stopped,true);
		if (stream)
		{
			if (audioStream)
				startTime = audioStream->getPlayedTime();
			stream->markFinished(false);
		}
		if(audioDecoder)
		{
			//Clear everything we have in buffers, discard all frames
			audioDecoder->setFlushing();
			audioDecoder->skipAll();
			audioDecoder=nullptr;
		}
		restartafterabort=true;
		startTime = starttime;
	}
	else
	{
		if (restartafterabort)
		{
			mutex.unlock();
			return;
		}
		mutex.unlock();
		while (!ACQUIRE_READ(terminated))
			getSystemState()->sleep_ms(10);
		mutex.lock();
		restartafterabort=false;
		startTime = starttime;
		if (ACQUIRE_READ(stopped))
		{
			// Start playback
			incRef();
			getSystemState()->addJob(this);
			RELEASE_WRITE(stopped,false);
			RELEASE_WRITE(terminated,false);
		}
	}
	mutex.unlock();
}
void SoundChannel::resume()
{
	if (ACQUIRE_READ(stopped))
	{
		// Start playback
		incRef();
		getSystemState()->addJob(this);
		RELEASE_WRITE(stopped,false);
		RELEASE_WRITE(terminated,false);
	}
}

void SoundChannel::markFinished()
{
	if (stream)
		stream->markFinished();
}

void SoundChannel::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable=true;
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(getPosition,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);

	REGISTER_GETTER_RESULTTYPE(c,leftPeak,Number);
	REGISTER_GETTER_RESULTTYPE(c,rightPeak,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,soundTransform,SoundTransform);
}

ASFUNCTIONBODY_GETTER(SoundChannel,leftPeak)
ASFUNCTIONBODY_GETTER(SoundChannel,rightPeak)
ASFUNCTIONBODY_SETTER_CB(SoundChannel,soundTransform,validateSoundTransform)
ASFUNCTIONBODY_ATOM(SoundChannel,_getter_soundTransform)
{
	if(!asAtomHandler::is<SoundChannel>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	SoundChannel* th = asAtomHandler::as<SoundChannel>(obj);
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	if (!th->soundTransform)
		th->soundTransform = _MR(Class<SoundTransform>::getInstanceS(wrk));
	th->soundTransform->incRef();
	ret = asAtomHandler::fromObject(th->soundTransform.getPtr());
}

void SoundChannel::finalize()
{
	EventDispatcher::finalize();
	stream.reset();
	if (sampleproducer)
		sampleproducer->removeStoredMember();
	sampleproducer=nullptr;
	starting=true;
	stopped=true;
	terminated=true;
	stopping=false;
	finished=false;
	audioDecoder=nullptr;
	audioStream=nullptr;
	format=AudioFormat(CODEC_NONE,0,0);
	soundinfo=nullptr;
	oldVolume=-1.0;
	startTime=0;
	loopstogo=0;
	streamposition=0;
	streamdatafinished=false;
	restartafterabort=false;
	forstreaming=false;
	fromSoundTag=nullptr;
	soundTransform.reset();
	leftPeak=1;
	rightPeak=1;
	threadAbort();
}

bool SoundChannel::destruct()
{
	stream.reset();
	if (sampleproducer)
		sampleproducer->removeStoredMember();
	sampleproducer=nullptr;
	starting=true;
	stopped=true;
	terminated=true;
	stopping=false;
	finished=false;
	audioDecoder=nullptr;
	audioStream=nullptr;
	format=AudioFormat(CODEC_NONE,0,0);
	soundinfo=nullptr;
	oldVolume=-1.0;
	startTime=0;
	loopstogo=0;
	streamposition=0;
	streamdatafinished=false;
	restartafterabort=false;
	forstreaming=false;
	fromSoundTag=nullptr;
	soundTransform.reset();
	leftPeak=1;
	rightPeak=1;
	threadAbort();
	return EventDispatcher::destruct();
}

void SoundChannel::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (soundTransform)
		soundTransform->prepareShutdown();
	if (sampleproducer)
		sampleproducer->prepareShutdown();
}

bool SoundChannel::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (sampleproducer)
		ret = sampleproducer->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void SoundChannel::validateSoundTransform(_NR<SoundTransform> oldValue)
{
	if (soundTransform.isNull())
	{
		soundTransform = oldValue;
		createError<TypeError>(getInstanceWorker(),kNullPointerError, "soundTransform");
	}
}

ASFUNCTIONBODY_ATOM(SoundChannel,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}

ASFUNCTIONBODY_ATOM(SoundChannel, stop)
{
	SoundChannel* th=asAtomHandler::as<SoundChannel>(obj);
	RELEASE_WRITE(th->stopping,true);
	th->threadAbort();
	while (!ACQUIRE_READ(th->stopped) && !ACQUIRE_READ(th->terminated))
		wrk->getSystemState()->sleep_ms(10);
}
ASFUNCTIONBODY_ATOM(SoundChannel,getPosition)
{
	if(!asAtomHandler::is<SoundChannel>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	SoundChannel* th = asAtomHandler::as<SoundChannel>(obj);
	asAtomHandler::setUInt(ret,wrk,th->audioStream ? th->audioStream->getPlayedTime() : th->startTime);
}
void SoundChannel::execute()
{
	// ensure audio manager is initialized
	getSystemState()->waitInitialized();
	for (loopstogo = (loopstogo < 1) ? 1 : loopstogo; loopstogo; --loopstogo)
	{
		mutex.lock();
		if (audioStream)
		{
			getSystemState()->audioManager->removeStream(audioStream);
			audioStream=nullptr;
		}
		mutex.unlock();
		RELEASE_WRITE(finished,false);
		if (sampleproducer && !sampleproducer->is<AVM1Sound>())
			playStreamFromSamples();
		else
			playStream();
		if (threadAborting || ACQUIRE_READ(stopped))
			break;
	}
	if (!loopstogo)
	{
		if (ACQUIRE_READ(finished))
		{
			EventDispatcher* ev = !sampleproducer || !sampleproducer->is<AVM1Sound>() ? (EventDispatcher*)this : (EventDispatcher*)sampleproducer;
			if (ev != nullptr)
			{
				ev->incRef();
				getVm(getSystemState())->addEvent(_MR(ev),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"soundComplete")));
			}
		}
	}
}

void SoundChannel::playStream()
{
	assert(!stream.isNull());
	std::streambuf *sbuf = stream->createReader();
	istream s(sbuf);
	s.exceptions ( istream::failbit | istream::badbit );
	bool waitForFlush=true;
	StreamDecoder* streamDecoder=nullptr;

	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
#ifdef ENABLE_LIBAVCODEC
		if(threadAborting)
			throw JobTerminationException();
		streamDecoder=new FFMpegStreamDecoder(nullptr,this->getSystemState()->getEngineData(),s,buffertimeseconds,&format,stream->hasTerminated() && !forstreaming ? stream->getReceivedLength() : -1);
		if(!streamDecoder->isValid())
		{
			LOG(LOG_ERROR,"invalid streamDecoder:"<<format.codec<<" "<<format.channels<<"/"<<format.sampleRate<<" "<<stream->getReceivedLength());
			threadAbort();
			restartafterabort=false;
		}
		else if (this->startTime!=0)
		{
			streamDecoder->jumpToPosition(this->startTime);
			if (audioStream)
				audioStream->setPlayedTime(this->startTime);
		}
		RELEASE_WRITE(starting,false);
		while(!ACQUIRE_READ(stopped))
		{
			bool decodingSuccess=streamDecoder->decodeNextFrame();
			if(decodingSuccess==false)
				break;
			if(audioDecoder==nullptr && streamDecoder->audioDecoder)
				audioDecoder=streamDecoder->audioDecoder;

			mutex.lock();
			if(threadAborting)
			{
				mutex.unlock();
				throw JobTerminationException();
			}
			if(audioStream==nullptr && audioDecoder && audioDecoder->isValid())
				audioStream=getSystemState()->audioManager->createStream(audioDecoder,false,this,this->fromSoundTag ? this->fromSoundTag->getId() : -1,startTime,soundTransform ? number_t(soundTransform->volume)/100.0 : 1.0);

			if(audioStream)
			{
				if (audioStream->getIsDone())
				{
					// stream was stopped by mixer
					getSystemState()->audioManager->removeStream(audioStream);
					audioStream=nullptr;
					RELEASE_WRITE(stopped,true);
					streamDecoder->audioDecoder->skipAll();
					waitForFlush=false;
					mutex.unlock();
					break;
				}
				if(soundTransform && soundTransform->volume != oldVolume)
				{
					audioStream->setVolume(number_t(soundTransform->volume)/100.0);
					oldVolume = soundTransform->volume;
				}
				checkEnvelope();
				if (soundTransform)
					audioStream->setPanning(soundTransform->leftToLeft, soundTransform->leftToRight, soundTransform->rightToRight, soundTransform->rightToLeft);
			}
			else if (audioDecoder && audioDecoder->isValid())
			{
				if (streamDecoder->atEnd())
				{
					audioDecoder->setFlushing();
					RELEASE_WRITE(stopped,true);
				}
				// no audiostream available, consume data anyway
				if (getSystemState()->getEngineData()->audio_useFloatSampleFormat())
				{
					float buf[512];
					audioDecoder->copyFrameF32(buf,512);
				}
				else
				{
					int16_t buf[512];
					audioDecoder->copyFrameS16(buf,512);
				}
			}
			mutex.unlock();
			if(threadAborting)
				throw JobTerminationException();
		}
#endif //ENABLE_LIBAVCODEC
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "Exception in SoundChannel " << e.cause);
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, "Exception in reading SoundChannel: "<<e.what());
	}
	if(waitForFlush)
	{
		//Put the decoders in the flushing state and wait for the complete consumption of contents
		if(audioStream && !audioStream->getIsDone() && streamDecoder && streamDecoder->audioDecoder)
		{
			streamDecoder->audioDecoder->setFlushing();
			streamDecoder->audioDecoder->waitFlushed();
		}
		if (!ACQUIRE_READ(stopping))
			RELEASE_WRITE(finished,true);// only add soundcomplete event if sound was played until the end
		else
			RELEASE_WRITE(stopping,false);
	}

	{
		mutex.lock();
		audioDecoder=nullptr;
		if (audioStream)
			getSystemState()->audioManager->removeStream(audioStream);
		audioStream=nullptr;
		mutex.unlock();
	}
	if (streamDecoder && streamDecoder->audioDecoder)
		streamDecoder->audioDecoder->skipAll();
	if (streamDecoder)
		delete streamDecoder;
	delete sbuf;
}

void SoundChannel::playStreamFromSamples()
{
	assert(stream.isNull());
	bool waitForFlush=true;
	SampleDataAudioDecoder* sampleDecoder = nullptr;
	//We need to catch possible EOF and other error condition in the non reliable stream
	try
	{
		sampleDecoder=new SampleDataAudioDecoder(this,100,getSystemState()->getEngineData());
		bool bufferfilled=false;
		while(!ACQUIRE_READ(stopped))
		{
			if(audioDecoder==nullptr)
				audioDecoder=sampleDecoder;
			if (!streamdatafinished && sampleproducer && sampleDecoder && sampleDecoder->getBufferedSamples()< sampleproducer->getBufferTime()*44100.0*2.0/1000.0)
			{
				if (sampleproducer->getSampleDataProcessed())
					sampleproducer->requestSampleDataEvent(streamposition);
			}
			else
			{
				bufferfilled=true;
				RELEASE_WRITE(starting,false);
			}
			if (bufferfilled)
			{
				if(audioStream==nullptr && audioDecoder && audioDecoder->isValid())
					audioStream=getSystemState()->audioManager->createStream(audioDecoder,false,this,-1,startTime,soundTransform ? soundTransform->volume : 1.0);
				
				if(audioStream)
				{
					if(soundTransform && soundTransform->volume != oldVolume)
					{
						audioStream->setVolume(number_t(soundTransform->volume)/100.0);
						oldVolume = soundTransform->volume;
					}
					checkEnvelope();
					if (soundTransform)
						audioStream->setPanning(soundTransform->leftToLeft, soundTransform->leftToRight, soundTransform->rightToRight, soundTransform->rightToLeft);
					if (streamdatafinished && !audioDecoder->hasDecodedFrames())
						threadAbort();
				}
				else if (audioDecoder && audioDecoder->isValid())
				{
					// no audio available, consume data anyway
					if (getSystemState()->getEngineData()->audio_useFloatSampleFormat())
					{
						float buf[512];
						audioDecoder->copyFrameF32(buf,512);
					}
					else
					{
						int16_t buf[512];
						audioDecoder->copyFrameS16(buf,512);
					}
				}
			}
			semSampleData.wait();
			if(threadAborting)
				throw JobTerminationException();
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "Exception in SoundChannel from samples: " << e.cause);
		threadAbort();
		waitForFlush=false;
	}
	catch(JobTerminationException& e)
	{
		waitForFlush=false;
	}
	catch(exception& e)
	{
		LOG(LOG_ERROR, "Exception in reading SoundChannel from samples: "<<e.what());
	}
	streamdatafinished=false;
	if(waitForFlush)
	{
		//Put the decoders in the flushing state and wait for the complete consumption of contents
		mutex.lock();
		if(audioStream && audioDecoder)
		{
			audioDecoder->setFlushing();
			audioDecoder->waitFlushed();
		}
		mutex.unlock();
		if (!ACQUIRE_READ(stopping))
			RELEASE_WRITE(finished,true);
		else
			RELEASE_WRITE(stopping,false);
	}

	mutex.lock();
	audioDecoder=nullptr;
	if (audioStream)
		getSys()->audioManager->removeStream(audioStream);
	audioStream=nullptr;
	mutex.unlock();
	if (sampleDecoder)
		delete sampleDecoder;
}


void SoundChannel::jobFence()
{
	mutex.lock();
	RELEASE_WRITE(stopped,true);
	RELEASE_WRITE(terminated,true);
	if (restartafterabort && !getSystemState()->isShuttingDown())
	{
		restartafterabort=false;
		incRef();
		getSystemState()->addJob(this);
		RELEASE_WRITE(stopped,false);
		RELEASE_WRITE(terminated,false);
	}
	mutex.unlock();
	this->decRef();
}

void SoundChannel::threadAbort()
{
	semSampleData.signal();
	mutex.lock();
	if (ACQUIRE_READ(stopped))
	{
		mutex.unlock();
		return;
	}
	RELEASE_WRITE(stopped,true);
	if (stream)
	{
		if (audioStream)
			startTime = audioStream->getPlayedTime();
		stream->markFinished(false);
	}
	if(audioDecoder)
	{
		//Clear everything we have in buffers, discard all frames
		audioDecoder->setFlushing();
		audioDecoder->skipAll();
		audioDecoder=nullptr;
	}
	mutex.unlock();
}
void SoundChannel::checkEnvelope()
{
	if (soundinfo && !soundinfo->SoundEnvelope.empty())
	{
		uint32_t playedtime = audioStream ? audioStream->getPlayedTime() : 0;
		auto itprev = soundinfo->SoundEnvelope.begin();
		for (auto it = soundinfo->SoundEnvelope.begin(); it != soundinfo->SoundEnvelope.end(); it++)
		{
			if (it->Pos44/44>playedtime)
				break;
			itprev=it;
		}
		if (itprev == soundinfo->SoundEnvelope.end())
			return;
		leftPeak= number_t(itprev->LeftLevel)*100.0/32768.0;
		rightPeak= number_t(itprev->RightLevel)*100.0/32768.0;
		if (soundTransform.isNull())
			soundTransform = _MR(Class<SoundTransform>::getInstanceSNoArgs(getInstanceWorker()));
		soundTransform->leftToLeft=leftPeak;
		soundTransform->rightToRight=rightPeak;
	}
}

void StageVideo::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("videoWidth","",c->getSystemState()->getBuiltinFunction(_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("videoHeight","",c->getSystemState()->getBuiltinFunction(_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",c->getSystemState()->getBuiltinFunction(attachNetStream),NORMAL_METHOD,true);
}


StageVideo::StageVideo(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),videoWidth(0),videoHeight(0)
{
}

void StageVideo::finalize()
{
	netStream.reset();
}

ASFUNCTIONBODY_ATOM(StageVideo,_getVideoWidth)
{
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	asAtomHandler::setUInt(ret,wrk,th->videoWidth);
}

ASFUNCTIONBODY_ATOM(StageVideo,_getVideoHeight)
{
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	asAtomHandler::setUInt(ret,wrk,th->videoHeight);
}

ASFUNCTIONBODY_ATOM(StageVideo,attachNetStream)
{
	StageVideo* th=asAtomHandler::as<StageVideo>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0])) //Drop the connection
	{
		Locker l(th->mutex);
		th->netStream=NullRef;
		return;
	}

	//Validate the parameter
	if(!asAtomHandler::is<NetStream>(args[0]))
		throw RunTimeException("Type mismatch in StageVideo::attachNetStream");

	//Acquire the netStream
	ASATOM_INCREF(args[0]);

	Locker l(th->mutex);
	th->netStream=_MR(asAtomHandler::as<NetStream>(args[0]));
}

void StageVideoAvailability::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("AVAILABLE","",abstract_s(c->getInstanceWorker(),"available"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",abstract_s(c->getInstanceWorker(),"unavailable"),DECLARED_TRAIT);
}

void VideoStatus::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("ACCELERATED","",abstract_s(c->getInstanceWorker(),"accelerated"),DECLARED_TRAIT);
	c->setVariableByQName("SOFTWARE","",abstract_s(c->getInstanceWorker(),"software"),DECLARED_TRAIT);
	c->setVariableByQName("UNAVAILABLE","",abstract_s(c->getInstanceWorker(),"unavailable"),DECLARED_TRAIT);
}
void Microphone::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER(c,isSupported);
	c->setDeclaredMethodByQName("getMicrophone","",c->getSystemState()->getBuiltinFunction(getMicrophone),NORMAL_METHOD,false);

}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Microphone,isSupported)

ASFUNCTIONBODY_ATOM(Microphone,getMicrophone)
{
	LOG(LOG_NOT_IMPLEMENTED,"Microphone.getMicrophone always returns null");
	asAtomHandler::setNull(ret);
}

void Camera::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER(c,isSupported);

}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Camera,isSupported)

void VideoStreamSettings::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("setKeyFrameInterval","",c->getSystemState()->getBuiltinFunction(setKeyFrameInterval),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMode","",c->getSystemState()->getBuiltinFunction(setMode),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings is a stub");
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,setKeyFrameInterval)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings.setKeyFrameInterval");
}
ASFUNCTIONBODY_ATOM(VideoStreamSettings,setMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoStreamSettings.setMode");
}

void H264VideoStreamSettings::sinit(Class_base* c)
{
	CLASS_SETUP(c, VideoStreamSettings, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("setProfileLevel","",c->getSystemState()->getBuiltinFunction(setProfileLevel),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, codec);
	REGISTER_GETTER(c, level);
	REGISTER_GETTER(c, profile);
}

ASFUNCTIONBODY_ATOM(H264VideoStreamSettings,_constructor)
{
	H264VideoStreamSettings* th =asAtomHandler::as<H264VideoStreamSettings>(obj);
	// Set default values
	th->codec = "H264Avc";
	th->profile = "baseline";
	th->level = "2.1";
}

ASFUNCTIONBODY_ATOM(H264VideoStreamSettings,setProfileLevel)
{
	LOG(LOG_NOT_IMPLEMENTED,"H264VideoStreamSettings.setProfileLevel");
	H264VideoStreamSettings* th =asAtomHandler::as<H264VideoStreamSettings>(obj);
	
	// TODO: Check inputs to see if they match H264Level and H264Profile

	ARG_CHECK(ARG_UNPACK(th->profile)(th->level));
}

ASFUNCTIONBODY_GETTER_SETTER(H264VideoStreamSettings, codec)
ASFUNCTIONBODY_GETTER(H264VideoStreamSettings, level)
ASFUNCTIONBODY_GETTER(H264VideoStreamSettings, profile)
