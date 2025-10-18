# Estimation of (mu,sigma) normal distribution from a clipped version
# www.overfitting.net
# https://www.overfitting.net/


################################

# WHAT HAPPENS TO SIGNAL AND SNR WHEN THE NORMAL DISTRBUTION GETS CLIPPED

BLACK=0
SAT=1

# Parche con (-12EV, 11dB) -> APENAS LE AFECTA EL TRUNCADO
set.seed(10)
EXP=SAT*(2^-12)
img=rnorm(1000000, mean=EXP , sd=0.00007)
hist(img, breaks=800, xlim=c(-0.001, 0.001), ylim=c(0,10000))
abline(v=0, col='red')
abline(v=EXP, col='red', lty='dotted')

S=mean(img)  # S=mean
N=var(img)^0.5  # N=stdev
SNR=S/N

Exp_EV=log2(S)
SNR_dB=20*log10(SNR)
print(paste0("S=", Exp_EV, "EV, S/N=", SNR_dB, "dB"))


# Parche con (-14EV, 0dB) -> LE AFECTA EL TRUNCADO
set.seed(10)
EXP=SAT*(2^-14)
img=rnorm(1000000, mean=EXP , sd=0.00006)
hist(img, breaks=800, xlim=c(-0.001, 0.001), ylim=c(0,10000))
abline(v=0, col='red')
abline(v=EXP, col='red', lty='dotted')

S=mean(img)  # S=mean
N=var(img)^0.5  # N=stdev
SNR=S/N

Exp_EV=log2(S)
SNR_dB=20*log10(SNR)
print(paste0("S=", Exp_EV, "EV, S/N=", SNR_dB, "dB"))


# Now clipped to 0
img2=img
img2[img2<0]=0
print(paste0(length(img2[img2==0])/length(img2)*100, "% of values clipped to 0"))
hist(img2, breaks=800, xlim=c(-0.001, 0.001), ylim=c(0,10000))
abline(v=0, col='red')
abline(v=EXP, col='red', lty='dotted')

S=mean(img2)  # S=mean
N=var(img2)^0.5  # N=stdev
SNR=S/N

Exp_EV=c(Exp_EV, log2(S))
SNR_dB=c(SNR_dB, 20*log10(SNR))
print(paste0("S=", Exp_EV, "EV, S/N=", SNR_dB, "dB"))
plot(Exp_EV, SNR_dB, xlab='Exposure (EV)', ylab='SNR (dB)',
     xlim=c(-14,0), ylim=c(-5,10))
arrows(Exp_EV[1], SNR_dB[1], Exp_EV[2], SNR_dB[2], length=0.15, col="blue", lwd=2)




##########################################
# ORIGINAL AND DERIVED CLIPPED DISTRIBUTION

set.seed(10)
MEAN=5
SD=3.5
TRUNC=2

img=rnorm(1000000, mean=MEAN , sd=SD)
hist(img, breaks=800, xlim=c(-20, 20))  # , ylim=c(0,10000))
abline(v=TRUNC, col='red')
abline(v=MEAN, col='red', lty='dotted')

img_clipped=img
img_clipped[img_clipped<TRUNC]=TRUNC
hist(img_clipped, breaks=800, xlim=c(-20, 20))  # , ylim=c(0,10000))
abline(v=TRUNC, col='red')
abline(v=MEAN, col='red', lty='dotted')



####################################
# METHOD 1. R base version - MIRRORING LOST PART OF GAUSS BELL
# NOTE: this method is only valid if at least half the original distribution remains

# Estimate mode as the peak of the density
density_mode <- function(x, bw = "nrd0") {
    d <- density(x, bw = bw)        # compute kernel density estimate
    d$x[which.max(d$y)]             # return x-value at density peak
}

img_clipped_R=img_clipped[img_clipped>TRUNC]
hist(img_clipped_R, breaks=800, xlim=c(-20, 20))  # , ylim=c(0,10000))
mode_val <- density_mode(img_clipped_R)
truncations=length(img[img<=TRUNC])

mu_hat=mode_val
abline(v=mu_hat, col='red', lty='dotted')

values_to_copy=img_clipped_R[order(img_clipped_R, decreasing = TRUE)[1:truncations]]
values_to_add=-(values_to_copy-(mu_hat+(mu_hat-TRUNC))) + TRUNC
values_to_add=values_to_add[values_to_add<=TRUNC]  # drop any intersection
img_reconstructed=c(img_clipped_R, values_to_add)
hist(img_reconstructed, breaks=1000, xlim=c(-20, 20))  # , ylim=c(0,10000))
abline(v=mu_hat, col='red', lty='dotted')

# Final estimation
print(paste0("METHOD 'mirroring': mean=", mean(img_reconstructed),
             ", sd=", sd(img_reconstructed)))



####################################
# METHOD 2. R base version - OPTIMIZATION

estimate_trunc_normal <- function(y, TRUNC = 0, mu_init = mean(y), sigma_init = sd(y)) {
    
    # Negative log-likelihood for left-censored/truncated normal
    neg_loglikelihood <- function(par, y, TRUNC) {
        mu <- par[1]
        sigma <- exp(par[2])  # enforce sigma > 0
        
        # we are not optimizing μ and σ directly but:
        #   mu: unconstrained
        #   log(sigma): unconstrained -> maps to sigma > 0
        # This trick makes the optimizer work as if both parameters are unconstrained,
        # while sigma is guaranteed positive by the exponential transformation

        # Censored and uncensored observations
        censored <- y <= TRUNC
        uncensored <- y > TRUNC
        
        n_cens <- sum(censored)
        pos <- y[uncensored]
        
        # Contribution of censored points
        ll_cens <- if(n_cens > 0) n_cens * log(pnorm((TRUNC - mu)/sigma)) else 0
        # Contribution of uncensored points
        ll_pos <- if(length(pos) > 0) sum(dnorm(pos, mean = mu, sd = sigma, log = TRUE)) else 0
        
        return(-(ll_cens + ll_pos))
    }
    
    # Optimize negative log-likelihood
    # y and TRUNC are extra arguments that are passed to fn
    # and come from calling estimate_trunc_normal(y, TRUNC,...)
    fit <- optim(par = c(mu_init, log(sigma_init)), fn = neg_loglikelihood, y = y, TRUNC = TRUNC,
                 method = "L-BFGS-B", hessian = TRUE)
    
    mu_hat <- fit$par[1]
    sigma_hat <- exp(fit$par[2])
    
    # list(mu = mu_hat, sigma = sigma_hat, loglik = -fit$value, convergence = fit$convergence)
    list(mu = mu_hat, sigma = sigma_hat)
}

estim1=estimate_trunc_normal(img_clipped, TRUNC = TRUNC)

# Final estimation
print(paste0("METHOD optim(): mean=", estim1$mu, ", sd=", estim1$sigma))



####################################
# METHOD 3. C++ version

library(Rcpp)

# IMPORTANT: I swear to God I once managed to compile the two functions
# in a single cppFunction() call, but later it didn't work: only nll_cpp() was
# created and I needed to use the 'includes' statement
cppFunction('List estimate_norm_censored_cpp(NumericVector y,
                                 double trunc_point,
                                 double mu_init,
                                 double sigma_init,
                                 int max_iter = 100,
                                 double lr = 1e-3,
                                 double tol = 1e-6) {
        // NOTE: Rcpp allows by default params but from the first specified default param
        // all other params need a default value. In this case these 3 params:
        // max_iter: number of iterations to stop
        // lr: learning rate controls the step size in each iteration of gradient descent
        // tol: tolerance determines when to stop iterating
        //      the loop breaks when the change in the negative log-likelihood is smaller than tol

        double mu = mu_init;
        double sigma = sigma_init;
        double prev = nll_cpp(y, trunc_point, mu, sigma);
    
        for(int iter = 0; iter < max_iter; iter++) {
            double h = 1e-5;
    
            // gradient wrt mu
            double gmu = (nll_cpp(y, trunc_point, mu + h, sigma) -
                          nll_cpp(y, trunc_point, mu - h, sigma)) / (2*h);
    
            // gradient wrt sigma
            double gsig = (nll_cpp(y, trunc_point, mu, sigma + h) -
                           nll_cpp(y, trunc_point, mu, sigma - h)) / (2*h);
    
            double new_mu = mu - lr*gmu;
            double new_sigma = sigma - lr*gsig;
            if(new_sigma <= 0) new_sigma = sigma;
    
            double curr = nll_cpp(y, trunc_point, new_mu, new_sigma);
    
            if(curr > prev) lr *= 0.5;
            else {
                mu = new_mu;
                sigma = new_sigma;
                if(std::fabs(prev - curr) < tol) break;
                prev = curr;
            }
        }
    
        return List::create(
            Named("mu") = mu,
            Named("sigma") = sigma
        );
    }',
    
    includes='double nll_cpp(const NumericVector& y, double trunc_point,
                   double mu, double sigma) {
        // Compute negative log-likelihood, this function is needed into the other
    
        int n = y.size();
        double val = 0.0;
        for(int i = 0; i < n; i++) {
            if(y[i] <= trunc_point) {
                double z = (trunc_point - mu) / sigma;
                double cdfv = R::pnorm(z, 0.0, 1.0, true, false);
                val += std::log(cdfv + 1e-16);
            } else {
                val += R::dnorm(y[i], mu, sigma, true);
            }
        }
        return -val;
    }
')
exists("estimate_norm_censored_cpp")

estim2=estimate_norm_censored_cpp(img_clipped, TRUNC,
                                  mean(img_clipped), sd(img_clipped))

# Final estimation
print(paste0("METHOD Rcpp: mean=", estim2$mu, ", sd=", estim2$sigma))
