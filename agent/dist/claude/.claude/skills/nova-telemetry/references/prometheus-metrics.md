# Prometheus Metrics Dictionary

## Metric Descriptors
| Metric Name | Type | Description |
|---|---|---|
| `novagraph_supersteps_total` | Counter | Total completed graph super-steps |
| `novagraph_node_executions_total` | Counter | Total executed agent nodes |
| `novagraph_node_failures_total` | Counter | Total failed node transitions |
| `novagraph_checkpoints_total` | Counter | Total persisted WAL snapshots |
| `novagraph_tokens_streamed_total` | Counter | Total streamed LLM tokens |
| `novagraph_execution_microseconds_total` | Counter | Total cumulative node execution time in microseconds |

## Prometheus Configuration
Add the NovaCPP job to `prometheus.yml`:
```yaml
scrape_configs:
  - job_name: 'novacpp'
    scrape_interval: 5s
    static_configs:
      - targets: ['localhost:8080']
```

