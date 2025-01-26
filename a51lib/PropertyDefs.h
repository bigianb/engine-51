#pragma once

// FIXME: this is not an enum

enum PropertyType
{
    PROP_TYPE_NULL     = 0,  
    PROP_TYPE_FLOAT    ,  
    PROP_TYPE_INT      ,  
    PROP_TYPE_BOOL     ,
    PROP_TYPE_VECTOR2  ,
    PROP_TYPE_VECTOR3  ,  
    PROP_TYPE_ROTATION ,  
    PROP_TYPE_ANGLE    ,  
    PROP_TYPE_BBOX     ,  
    PROP_TYPE_GUID     ,  
    PROP_TYPE_COLOR    ,  
    PROP_TYPE_STRING   ,  
    PROP_TYPE_ENUM     ,  
    PROP_TYPE_BUTTON   ,  
    PROP_TYPE_EXTERNAL ,  
    PROP_TYPE_FILENAME ,  

    PROP_TYPE_BASIC_MASK = 0xff,

    PROP_TYPE_HEADER            = (1<<19), 
    PROP_TYPE_READ_ONLY         = (1<<20), 
    PROP_TYPE_MUST_ENUM         = (1<<21),  
    PROP_TYPE_DONT_SAVE         = (1<<22), 
    PROP_TYPE_DONT_SHOW         = (1<<23), 
    PROP_TYPE_DONT_EXPORT       = (1<<24), 
    PROP_TYPE_EXPOSE            = (1<<25),   
    PROP_TYPE_DONT_SAVE_MEMCARD = (1<<26),
    PROP_TYPE_DONT_COPY         = (1<<27),   
};