//
//  FrameRate.m
//  freej
//
//  Created by xant on 3/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "FrameRate.h"

// GOT From apple example : QTQuartzPlayer

#pragma mark FrameRate
@implementation FrameRate
-(id)init
{
  //[super init];
  _count = 0;
  rate = 0;
  _frequency = CVGetHostClockFrequency();
 // lock = [[NSRecursiveLock alloc] init];
 // [lock retain]; 
  return self;
}

-(void)dealloc
{
    //[lock release];
    [super dealloc];
}

-(void)tick:(uint64_t)timestamp;
{
  int i = 0;
  //[lock lock];
  if (_count > NumStamps) {
      for (i = 0; i < _count; i++) {
        _stamps[i] = _stamps[i+1];
      }
      _count = NumStamps;  
  }
  _stamps[_count++] = timestamp;

      //[lock unlock];
}

-(double)rate
{
    //[lock lock];
    if (_count > 1)  {
        rate = 1000000000/((_stamps[_count - 1] - _stamps[0])/_count);
    }
    //[lock unlock];
    return rate;
}

@end

