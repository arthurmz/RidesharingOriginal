***************************************************************************************************************************************************************************************************************
Instance Name: RMZ_[L|R]P
Z: the total number of ridesharing requests (drivers + riders).
L: means that the drivers travels for long distances.
R: means drivers are randomly selected (not necessarily traveling long distances).
P: is the time window +-P.

***************************************************************************************************************************************************************************************************************
Req_no                           : each request is referenced by a unique number. For an instance with x drivers' requests, the first x requests are drivers' requests and the rest are riders' requests.
request_arrival_time             : The time at which the request is received.  request_arrival_time  should be less than or equal Pickup_earliest_time. For a given % of known riders' requests, the first y 
			           of the riders requests that satisfy the percent are considered the known riders requests. For all drivers requests and all known riders requests you can consider request_arrival_time=0.
service_time_at_source           : the required time to perform the pickup operation. considered 0 in all instances and for all requests.
Pickup_location_longitude 	 : Coordinates of the pickup location of the request
Pickup_location_latitude  	 : Coordinates of the pickup location of the request
Pickup_earliest_time 		 : [a,] part of the time window for the pickup 
Pickup_latest_time               : [,b] part of the time window for the pickup 
service_time_at_delivery         : the required time to perform the delivery operation. considered 0 in all instances and for all requests.
delivery_location_long           : Coordinates of the delivery location of the request
delivery_location_latitude       : Coordinates of the delivery location of the request
delivery_earliest_time           : [a,] part of the time window for the delivery
delivery_latest_time             : [,b] part of the time window for the delivery