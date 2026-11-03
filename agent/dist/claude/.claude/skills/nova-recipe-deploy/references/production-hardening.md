# Production Hardening & Cluster Deployment

## Kubernetes Deployment Blueprint
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: novacpp-backend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: novacpp
  template:
    metadata:
      labels:
        app: novacpp
    spec:
      containers:
      - name: app
        image: ghcr.io/org/novacpp-backend:latest
        resources:
          requests:
            cpu: "250m"
            memory: "128Mi"
          limits:
            cpu: "1000m"
            memory: "512Mi"
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 3
          periodSeconds: 10
        env:
        - name: PORT
          value: "8080"
        - name: REDIS_URL
          valueFrom:
            secretKeyRef:
              name: cluster-secrets
              key: redis-url
```

## Security Checklist
1. Non-root user context (`runAsNonRoot: true`).
2. Read-only root filesystem with ephemeral `/tmp` volume.
3. TLS termination managed at Ingress or Envoy reverse proxy layer.

