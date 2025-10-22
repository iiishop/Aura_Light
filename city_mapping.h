#ifndef CITY_MAPPING_H
#define CITY_MAPPING_H

struct CityMapping
{
    const char *ip138Name; 
    const char *cityName;  
};

const CityMapping CITY_MAP[] = {
    
    
    

    
    {"北京", "Beijing"},
    {"上海", "Shanghai"},
    {"天津", "Tianjin"},
    {"重庆", "Chongqing"},

    
    {"深圳", "Shenzhen"},
    {"厦门", "Xiamen"},

    
    {"广州", "Guangzhou"},      
    {"杭州", "Hangzhou"},       
    {"南京", "Nanjing"},        
    {"成都", "Chengdu"},        
    {"武汉", "Wuhan"},          
    {"西安", "Xi'an"},          
    {"福州", "Fuzhou"},         
    {"济南", "Jinan"},          
    {"郑州", "Zhengzhou"},      
    {"长沙", "Changsha"},       
    {"哈尔滨", "Harbin"},       
    {"沈阳", "Shenyang"},       
    {"昆明", "Kunming"},        
    {"南昌", "Nanchang"},       
    {"合肥", "Hefei"},          
    {"石家庄", "Shijiazhuang"}, 
    {"太原", "Taiyuan"},        
    {"长春", "Changchun"},      
    {"南宁", "Nanning"},        
    {"贵阳", "Guiyang"},        
    {"兰州", "Lanzhou"},        
    {"呼和浩特", "Hohhot"},     
    {"乌鲁木齐", "Urumqi"},     
    {"银川", "Yinchuan"},       
    {"西宁", "Xining"},         
    {"拉萨", "Lhasa"},          
    {"海口", "Haikou"},         

    
    {"苏州", "Suzhou"},  
    {"青岛", "Qingdao"}, 
    {"大连", "Dalian"},  

    
    {"香港", "Hong Kong"},
    {"澳门", "Macau"},
    {"台北", "Taipei"},

    
    
    

    
    {"伦敦", "London"},

    
    {"纽约", "New York"},
    {"洛杉矶", "Los Angeles"},
    {"旧金山", "San Francisco"},
    {"芝加哥", "Chicago"},
    {"西雅图", "Seattle"},
    {"波士顿", "Boston"},
    {"华盛顿", "Washington"},
    {"迈阿密", "Miami"},
    {"拉斯维加斯", "Las Vegas"},

    
    {"巴黎", "Paris"},

    
    {"柏林", "Berlin"},
    {"法兰克福", "Frankfurt"},
    {"慕尼黑", "Munich"},

    
    {"阿姆斯特丹", "Amsterdam"},

    
    {"布鲁塞尔", "Brussels"},

    
    {"苏黎世", "Zurich"},

    
    {"维也纳", "Vienna"},

    
    {"罗马", "Rome"},
    {"米兰", "Milan"},

    
    {"马德里", "Madrid"},
    {"巴塞罗那", "Barcelona"},

    
    {"莫斯科", "Moscow"},
    {"圣彼得堡", "Saint Petersburg"},

    
    {"迪拜", "Dubai"},

    
    {"东京", "Tokyo"},

    
    {"首尔", "Seoul"},

    
    {"新加坡", "Singapore"},

    
    {"悉尼", "Sydney"},
    {"墨尔本", "Melbourne"},

    
    {"多伦多", "Toronto"},
    {"温哥华", "Vancouver"},

    
    {"曼谷", "Bangkok"},

    
    {"吉隆坡", "Kuala Lumpur"},

    
    {"雅加达", "Jakarta"},

    
    {"马尼拉", "Manila"},

    
    {"孟买", "Mumbai"},
    {"新德里", "New Delhi"},
    {"班加罗尔", "Bangalore"},

    
    {"开罗", "Cairo"},

    
    {"约翰内斯堡", "Johannesburg"},

    
    {"圣保罗", "Sao Paulo"},
    {"里约热内卢", "Rio de Janeiro"},

    
    {"墨西哥城", "Mexico City"},

    
    {"布宜诺斯艾利斯", "Buenos Aires"},

    
    {"奥克兰", "Auckland"},
};

const int CITY_MAP_SIZE = sizeof(CITY_MAP) / sizeof(CITY_MAP[0]);

#endif 
