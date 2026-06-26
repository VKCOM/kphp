---types---

// Metric

metricValue#0cb5eb87 value:double = MetricValueFormat;
metricValuesArray#d4a59582 values:%(Vector double) = MetricValueFormat;
metricCount#941bf7d1 count:int = MetricValueFormat;
metricInc#23e305ab = MetricValueFormat;

metric timestamp:long value:MetricValueFormat matric_name:string tags:%(Vector %(Pair string string)) = Metric;
