---types---

pair {X:Type} {Y:Type} a:X b:Y = Pair X Y;
span#dad3ae87 {t:Type} count:# data:count*[t] = Span t;

// Metric

metricValue#0cb5eb87 value:double = MetricValue;
metricValuesArray#d4a59582 values:%(Span double) = MetricValue;
metricCount#941bf7d1 count:u32 = MetricValue;
metricInc#23e305ab = MetricValue;

metric#87d62ee3 timestamp:u64 value:MetricValue metric_name:string tags:%(Span %(Pair string string)) = Metric;
