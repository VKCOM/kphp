---types---

pair {X:Type} {Y:Type} a:X b:Y = Pair X Y;
span#dad3ae87 {t:Type} count:# data:count*[t] = Span t;

// Metric

metricValue#0cb5eb87 value:double = AnyMetricValue;
metricValuesArray#d4a59582 values:%(Span double) = AnyMetricValue;
metricCount#941bf7d1 count:u32 = AnyMetricValue;
metricInc#23e305ab = AnyMetricValue;

metric#87d62ee3 timestamp:u64 value:AnyMetricValue metric_name:string tags:%(Span %(Pair string string)) = Metric;
